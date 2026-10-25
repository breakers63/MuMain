#include <doctest.h>
#include "UI/RmlBridge/RmlPasswordField.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

namespace
{
class PasswordSystem : public Rml::SystemInterface
{
public:
    bool LogMessage(Rml::Log::Type, const Rml::String&) override { return true; }
};

class PasswordRenderer : public Rml::RenderInterface
{
public:
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return 0; }
    void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
    void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
    Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override { return 0; }
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return 0; }
    void ReleaseTexture(Rml::TextureHandle) override {}
    void EnableScissorRegion(bool) override {}
    void SetScissorRegion(Rml::Rectanglei) override {}
};

bool ContainsDisplayText(Rml::Element* field, const Rml::String& expected)
{
    for (int index = 0; index < field->GetNumChildren(true); ++index)
        if (auto* text = rmlui_dynamic_cast<Rml::ElementText*>(field->GetChild(index)))
            if (text->GetText() == expected)
                return true;
    return false;
}
}

TEST_CASE("Password eye click reveals the shared form value and preserves subsequent typing")
{
    PasswordRenderer renderer;
    PasswordSystem system;
    Rml::SetSystemInterface(&system);
    Rml::SetRenderInterface(&renderer);
    REQUIRE(Rml::Initialise());
    Rml::Context* context = Rml::CreateContext("password-eye", {640, 480});
    REQUIRE(context != nullptr);
    Rml::String password = "Paris42";
    bool visible = false;
    int clicks = 0;
    Rml::ElementDocument* document = nullptr;
    auto model = context->CreateDataModel("login_eye");
    REQUIRE(model);
    model.Bind("password", &password);
    model.BindEventCallback("toggle_eye", [&](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        visible = !visible;
        ++clicks;
        UI::RmlBridge::SetPasswordVisible(document->GetElementById("masked"), document->GetElementById("plain"), visible);
    });
    document = context->LoadDocumentFromMemory(
        "<rml><head><style>body{width:640px;height:480px;}input{position:absolute;left:10px;top:10px;width:150px;height:30px;}"
        "input.password-hidden{display:none;}#eye{position:absolute;left:180px;top:10px;width:30px;height:30px;pointer-events:auto;}"
        "</style></head><body data-model='login_eye'>"
        "<input id='masked' type='password' data-value='password' maxlength='12'/>"
        "<input id='plain' type='text' class='password-hidden' data-value='password' maxlength='12'/>"
        "<button id='eye' data-event-click='toggle_eye'></button></body></rml>");
    REQUIRE(document != nullptr);
    document->Show();
    context->Update();
    auto* masked = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(document->GetElementById("masked"));
    auto* plain = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(document->GetElementById("plain"));
    REQUIRE(masked != nullptr);
    REQUIRE(plain != nullptr);
    CHECK(ContainsDisplayText(masked, "*******"));
    context->ProcessMouseMove(195, 25, 0);
    context->ProcessMouseButtonDown(0, 0);
    context->ProcessMouseButtonUp(0, 0);
    context->Update();
    CHECK(clicks == 1);
    CHECK(visible);
    CHECK_FALSE(masked->IsVisible());
    CHECK(plain->IsVisible());
    CHECK(ContainsDisplayText(plain, "Paris42"));
    CHECK(password == "Paris42");
    plain->SetValue("");
    Rml::Dictionary changed;
    changed["value"] = Rml::String("");
    plain->DispatchEvent(Rml::EventId::Change, changed);
    context->Update();
    Rml::Dictionary typing;
    typing["text"] = Rml::String("abcdefghijklmnop");
    plain->DispatchEvent(Rml::EventId::Textinput, typing);
    context->Update();
    CHECK(password == "abcdefghijkl");
    CHECK(masked->GetValue() == password);
    context->ProcessMouseMove(195, 25, 0);
    context->ProcessMouseButtonDown(0, 0);
    context->ProcessMouseButtonUp(0, 0);
    context->Update();
    CHECK(clicks == 2);
    CHECK_FALSE(visible);
    CHECK(masked->IsVisible());
    CHECK_FALSE(plain->IsVisible());
    CHECK(ContainsDisplayText(masked, "************"));
    CHECK(password == "abcdefghijkl");
    Rml::RemoveContext("password-eye");
    Rml::Shutdown();
    Rml::SetRenderInterface(nullptr);
    Rml::SetSystemInterface(nullptr);
}

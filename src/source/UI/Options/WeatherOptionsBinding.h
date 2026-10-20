#pragma once
#include <functional>
namespace Rml
{
class DataModelConstructor;
}
namespace UI::Options::Weather
{
void Bind(Rml::DataModelConstructor& model, std::function<void(const char*)> dirty);
}

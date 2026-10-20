Texture2D<float4> sceneTexture : register(t0, space2);
SamplerState sceneSampler : register(s0, space2);
cbuffer HdSettings : register(b0, space3)
{
    float2 texel;
    float sharpness;
    float bloom;
    float exposure;
    float colorGrading;
    float2 padding;
};
float3 SampleScene(float2 uv)
{
    return sceneTexture.Sample(sceneSampler, uv).rgb;
}
float3 Highlight(float3 color)
{
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    return color * smoothstep(0.64, 1.0, luminance);
}
struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};
float4 main(Input input) : SV_Target0
{
    float2 uv = input.uv;
    float4 center = sceneTexture.Sample(sceneSampler, uv);
    float3 north = SampleScene(uv + float2(0, -texel.y));
    float3 south = SampleScene(uv + float2(0, texel.y));
    float3 east = SampleScene(uv + float2(texel.x, 0));
    float3 west = SampleScene(uv + float2(-texel.x, 0));
    float3 low = min(center.rgb, min(min(north, south), min(east, west)));
    float3 high = max(center.rgb, max(max(north, south), max(east, west)));
    float3 sharpened = center.rgb + (center.rgb - (north + south + east + west) * 0.25) * sharpness;
    float3 color = clamp(sharpened, low - 0.025, high + 0.025);
    if (bloom > 0.0)
    {
        float3 glow = Highlight(center.rgb) * 0.20;
        const float2 directions[8] = {
            float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1),
            float2(0.7071, 0.7071), float2(-0.7071, 0.7071),
            float2(0.7071, -0.7071), float2(-0.7071, -0.7071)};
        [unroll] for (int tap = 0; tap < 8; ++tap)
            glow += Highlight(SampleScene(uv + directions[tap] * texel * 4.0)) * 0.10;
        color += glow * bloom * (1.0 - saturate(color));
    }
    color *= exposure;
    if (colorGrading > 0.0)
    {
        float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
        color = lerp(luminance.xxx, color, 1.08);
        color = (color - 0.5) * 1.035 + 0.5;
    }
    return float4(saturate(color), center.a);
}

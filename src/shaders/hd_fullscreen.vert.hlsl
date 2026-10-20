struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};
Output main(uint vertex : SV_VertexID)
{
    Output result;
    // One oversized triangle avoids a seam through the middle of the screen.
    result.uv = float2((vertex << 1) & 2, vertex & 2);
    result.position = float4(result.uv.x * 2.0 - 1.0, 1.0 - result.uv.y * 2.0, 0.0, 1.0);
    return result;
}

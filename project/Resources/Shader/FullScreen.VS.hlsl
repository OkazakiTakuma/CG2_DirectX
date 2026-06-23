#include "FullScreen.hlsli"

static const uint kNumVertex = 3;
static const float4 kPositions[kNumVertex] =
{
    float4(-1.0f, 1.0f, 0.0f, 1.0f),
    float4(3.0f, 1.0f, 0.0f, 1.0f),
    float4(-1.0f, -3.0f, 0.0f, 1.0f)
};

static const float2 kTexCoords[kNumVertex] =
{
    float2(0.0f, 0.0f),
    float2(2.0f, 0.0f),
    float2(0.0f, 2.0f)
};

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    VertexShaderOutput output;
    output.pos = kPositions[vertexID];
    output.uv = kTexCoords[vertexID];
    return output;
}
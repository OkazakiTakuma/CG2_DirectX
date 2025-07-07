#include"Object3D.hlsli"

cbuffer WVP : register(b1)
{
    matrix wvpMatrix;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;

};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, wvpMatrix); // ← 行列を使って変換
    output.texcoord = input.texcoord;
    return output;
}

#include "Object3d.hlsli"

// 頂点は既に GPU 側でスキンされたものを受け取る（position, texcoord, normal）
struct VertexShaderInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

// カメラやオブジェクトの行列
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    float4 pos4 = float4(input.position, 1.0f);
    output.position = mul(pos4, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(pos4, gTransformationMatrix.world).xyz;

    return output;
}

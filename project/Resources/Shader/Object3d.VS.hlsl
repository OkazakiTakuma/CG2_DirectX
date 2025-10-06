#include"Object3D.hlsli"


struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1); // Material constant buffer

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;


};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP); // ← 行列を使って変換
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.world));
    return output;
}

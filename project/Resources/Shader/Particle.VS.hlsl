#include"Particle.hlsli"


struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t0); // Material constant buffer

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;


};

VertexShaderOutput main(VertexShaderInput input,uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix[instanceID].WVP); // ← 行列を使って変換
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix[instanceID].world));
    return output;
}

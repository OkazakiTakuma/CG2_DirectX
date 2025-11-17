#include"Object3D.hlsli"


struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t1); // Material constant buffer

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;


};

VertexShaderOutput main(VertexShaderInput input,int instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrices[instanceID].WVP); // ← 行列を使って変換
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices[instanceID].world));
    return output;
}
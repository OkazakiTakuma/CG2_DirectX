#include"particle.hlsli"


struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 world;
    float4 color;
};
StructuredBuffer<ParticleForGPU> gParticle : register(t1); // Material constant buffer

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;


};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    output.position = mul(input.position, gParticle[instanceID].WVP); // ← 行列を使って変換
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gParticle[instanceID].world));
    output.color = gParticle[instanceID].color;
    return output;
}
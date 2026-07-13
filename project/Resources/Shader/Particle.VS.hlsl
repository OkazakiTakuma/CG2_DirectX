#include "Particle.hlsli"

struct ParticleForGPU
{
    float3 translate;
    float isBillboard;
    float3 scale;
    float padding0;
    float3 rotate;
    float padding1;
    float4 color;
};

struct ParticleScene
{
    float4x4 viewProjection;
    float4x4 billboard;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t1);
ConstantBuffer<ParticleScene> gScene : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

float4x4 MakeScaleMatrix(float3 scale)
{
    return float4x4(
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeRotateXMatrix(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeRotateYMatrix(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float4x4(
        c, 0.0f, -s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeRotateZMatrix(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float4x4(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeTranslateMatrix(float3 translate)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        translate.x, translate.y, translate.z, 1.0f
    );
}

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    ParticleForGPU particle = gParticle[instanceID];

    float4x4 scaleMatrix = MakeScaleMatrix(particle.scale);
    float4x4 rotateMatrix = mul(mul(MakeRotateXMatrix(particle.rotate.x), MakeRotateYMatrix(particle.rotate.y)), MakeRotateZMatrix(particle.rotate.z));
    float4x4 rotationBase = particle.isBillboard > 0.5f ? mul(rotateMatrix, gScene.billboard) : rotateMatrix;
    float4x4 world = mul(mul(scaleMatrix, rotationBase), MakeTranslateMatrix(particle.translate));

    output.position = mul(mul(input.position, world), gScene.viewProjection);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3)world));
    output.color = particle.color;
    return output;
}

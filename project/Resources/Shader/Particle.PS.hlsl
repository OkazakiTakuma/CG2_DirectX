#include "Particle.hlsli"
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
struct Material
{
    float4 color; // Color of the material
    int enableLighting; // Flag to enable lighting
    float4x4 uvTransform; // UV transformation matrix   
};

struct DirectionalLight
{
   
    float4 color; // Color of the light
    float3 direction; // Direction of the light
    float intensity; // Intensity of the light
    
};
ConstantBuffer<Material> gMaterial : register(b0); // Material constant buffer
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2); // Directional light constant buffer
struct PixelShaderOutput
{
    float4 color : SV_Target0; // Output color of the pixel shader
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    output.color = textureColor * gMaterial.color;
    if(output.color.a == 0)
    {
        discard;
    }
   
    return output;
}
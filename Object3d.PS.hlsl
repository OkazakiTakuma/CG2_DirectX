#include "Object3d.hlsli"
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
struct Material
{
    float4 color; // Color of the material
};

ConstantBuffer<Material> gMaterial : register(b0); // Material constant buffer
struct PixelShaderOutput
{
    float4 color : SV_Target0; // Output color of the pixel shader
};

PixelShaderOutput main(VertexShaderOutput input)
{
  
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    PixelShaderOutput output;
    output.color = gMaterial.color*textureColor; // Use the material color as the output color
    return output;
}
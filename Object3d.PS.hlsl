#include "Object3d.hlsli"
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
struct Material
{
    float4 color; // Color of the material
    int enableLighting; // Flag to enable lighting
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
  
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    PixelShaderOutput output;
    if (gMaterial.enableLighting!= 0)
    {
        // 光と法線の内積 → cosθ 相当
        float cosLight = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));

        // ライトによる色変化
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cosLight * gDirectionalLight.intensity;
      
    }
    else
    {
        // ライティングなしで色をそのまま合成
        output.color = gMaterial.color * textureColor;
    }
    return output;
}
#include "Object3d.hlsli"
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
// --- Object3d.PS.hlsl ---
struct Material
{
    float4 color; // 16バイト
    int enableLighting; // 4バイト
    float3 padding; // 12バイト (これで合計32バイト、16の倍数)
    float4x4 uvTransform; // 64バイト (合計96バイト)
    float shininess; // 4バイト
    float3 padding2; // 12バイト (合計112バイト)
};
struct Camera
{
    float3 worldPosition; // World position of the camera
};

struct DirectionalLight
{
   
    float4 color; // Color of the light
    float3 direction; // Direction of the light
    float intensity; // Intensity of the light
    
};
ConstantBuffer<Material> gMaterial : register(b0); // Material constant buffer
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2); // Directional light constant buffer
ConstantBuffer<Camera> gCamera : register(b3); // Camera constant buffer
struct PixelShaderOutput
{
    float4 color : SV_Target0; // Output color of the pixel shader
};

PixelShaderOutput main(VertexShaderOutput input)
{
    
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
    float RdotE = dot(reflectLight, toEye);
    float specularPow = pow(saturate(RdotE), gMaterial.shininess);
    if(textureColor.a == 0.0)
    {
        discard; // 透明度が低いピクセルは描画しない
    }
    if(textureColor.a <=0.5)
    {
       discard; // 透明度が低いピクセルは描画しない
    }

    PixelShaderOutput output;
    if (gMaterial.enableLighting != 0)
    {
        // 光と法線の内積 → cosθ 相当
        float NdotL = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        // ライトによる色変化
        float3 diffuse = gMaterial.color.rgb* textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        float3 specular = gDirectionalLight.color.rgb * specularPow * gDirectionalLight.intensity * float3(1.0f,1.0f,1.0f);
        
        output.color.rgb = diffuse + specular;
        
        
        
        output.color.a = gMaterial.color.a * textureColor.a;
        if(output.color.a == 0.0)
        {
            discard; // 透明度が低いピクセルは描画しない
        }
    
      
    }
    else
    {
        // ライティングなしで色をそのまま合成
        output.color= gMaterial.color * textureColor;
    }
    return output;
}
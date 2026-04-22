#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess; // ★追加：光沢の強さ
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// ★追加：カメラ座標用
struct CameraInfo
{
    float3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<CameraInfo> gCamera : register(b3); // ★b3を使用

struct PixelShaderOutput
{
    float4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    if (textureColor.a <= 0.5)
        discard;

    PixelShaderOutput output;

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal); // 法線
        float3 L = normalize(-gDirectionalLight.direction); // ライトへの方向
        float3 V = normalize(gCamera.worldPosition - input.worldPosition); // 視点への方向
        float3 R = reflect(-L, N); // 反射ベクトル

        // 1. Ambient (環境反射)
        float3 ambient = gMaterial.color.rgb * textureColor.rgb * 0.1f;

        // 2. Diffuse (拡散反射)
        float NdotL = saturate(dot(N, L));
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;

        // 3. Specular (鏡面反射) - Phong方式
        float RdotV = saturate(dot(R, V));
        float specularPow = pow(RdotV, gMaterial.shininess);
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow;

        output.color.rgb = ambient + diffuse + specular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    return output;
}
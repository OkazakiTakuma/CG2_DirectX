#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
    float shininess;
    float3 padding2;
};

struct Camera
{
    float3 worldPosition;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// --- 追加: PointLight構造体 ---
struct PointLight
{
    float4 color; // 色
    float3 position; // 位置
    float intensity; // 輝度
    float radius; // ライトの届く最大距離
    float decay; // 減衰率
    float2 padding; // パディング
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);

// --- 追加: レジスタ b4 に割り当て ---
ConstantBuffer<PointLight> gPointLight : register(b4);

struct PixelShaderOutput
{
    float4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // アルファテスト
    if (textureColor.a <= 0.5)
    {
        discard;
    }

    // ライティングが無効な場合
    if (gMaterial.enableLighting == 0)
    {
        PixelShaderOutput output;
        output.color = gMaterial.color * textureColor;
        return output;
    }

    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float3 normal = normalize(input.normal);

    // --- Directional Light (既存の処理) ---
    float NdotL = saturate(dot(normal, -gDirectionalLight.direction));
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float3 directionalDiffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    
    float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float specularPow = pow(saturate(NdotH), gMaterial.shininess);
    float3 directionalSpecular = gDirectionalLight.color.rgb * specularPow * gDirectionalLight.intensity;

    // --- 追加: Point Light 計算 ---
    // ライトへのベクトルと距離
    float3 lightVec = gPointLight.position - input.worldPosition;
    float distance = length(lightVec);
    float3 pointLightDir = normalize(lightVec);

    // 距離減衰（逆二乗の法則や線形減衰など。ここでは半径に基づく簡易的な減衰を採用）
    float factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);

    // 拡散反射 (Diffuse)
    float NdotL_Point = saturate(dot(normal, pointLightDir));
    float3 pointDiffuse = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * NdotL_Point * gPointLight.intensity * factor;

    // 鏡面反射 (Specular)
    float3 halfVectorPoint = normalize(pointLightDir + toEye);
    float NdotH_Point = saturate(dot(normal, halfVectorPoint));
    float specularPowPoint = pow(saturate(NdotH_Point), gMaterial.shininess);
    float3 pointSpecular = gPointLight.color.rgb * specularPowPoint * gPointLight.intensity * factor;

    // --- 色の合成 ---
    PixelShaderOutput output;
    output.color.rgb = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;
    output.color.a = gMaterial.color.a * textureColor.a;

    return output;
}
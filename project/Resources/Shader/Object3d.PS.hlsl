#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// --- 変更: Material構造体に制御フラグを追加 ---
struct Material
{
    float4 color;
    int enableLighting;
    // 元の float3 padding を分解してフラグに使用
    // HLSLのパッキングルールに合わせるため、float4x4の前までの隙間を利用
    int enableDiffuse; // 拡散反射の有効/無効
    int enableSpecular; // 鏡面反射の有効/無効
    float padding; // アライメント調整用 (合計32バイト境界を維持)
    
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
    float intensity; // 強度
    float radius; // 半径
    float decay; // 減衰率
    float2 padding; // パディング
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);

// --- 追加: レジスタ b4 ---
ConstantBuffer<PointLight> gPointLight : register(b4);

struct PixelShaderOutput
{
    float4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a <= 0.5)
    {
        discard;
    }

    if (gMaterial.enableLighting == 0)
    {
        PixelShaderOutput output;
        output.color = gMaterial.color * textureColor;
        return output;
    }

    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float3 normal = normalize(input.normal);

    // ライティング結果の初期化
    float3 totalDiffuse = float3(0.0f, 0.0f, 0.0f);
    float3 totalSpecular = float3(0.0f, 0.0f, 0.0f);

    // ==========================================
    // 1. Directional Light 計算
    // ==========================================
    if (gMaterial.enableDiffuse != 0)
    {
        float NdotL = saturate(dot(normal, -gDirectionalLight.direction));
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        totalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    }
    
    if (gMaterial.enableSpecular != 0)
    {
        float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
        float NdotH = saturate(dot(normal, halfVector));
        float specularPow = pow(saturate(NdotH), gMaterial.shininess);
        totalSpecular += gDirectionalLight.color.rgb * specularPow * gDirectionalLight.intensity;
    }

    // ==========================================
    // 2. Point Light 計算 (追加)
    // ==========================================
    float3 lightVec = gPointLight.position - input.worldPosition;
    float distance = length(lightVec);
    float3 pointLightDir = normalize(lightVec);
    
    // 距離減衰 (半径に基づく簡易計算)
    float factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);

    if (gMaterial.enableDiffuse != 0)
    {
        float NdotL_Point = saturate(dot(normal, pointLightDir));
        totalDiffuse += gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * NdotL_Point * gPointLight.intensity * factor;
    }

    if (gMaterial.enableSpecular != 0)
    {
        float3 halfVectorPoint = normalize(pointLightDir + toEye);
        float NdotH_Point = saturate(dot(normal, halfVectorPoint));
        float specularPowPoint = pow(saturate(NdotH_Point), gMaterial.shininess);
        totalSpecular += gPointLight.color.rgb * specularPowPoint * gPointLight.intensity * factor;
    }

    // ==========================================
    // 合成出力
    // ==========================================
    PixelShaderOutput output;
    output.color.rgb = totalDiffuse + totalSpecular;
    output.color.a = gMaterial.color.a * textureColor.a;

    return output;
}
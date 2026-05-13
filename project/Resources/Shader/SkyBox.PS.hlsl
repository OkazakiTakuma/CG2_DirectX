#include "SkyBox.hlsli"

TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess; // 光沢の強さ
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// カメラ座標用
struct CameraInfo
{
    float3 worldPosition;
};

// ★追加：ポイントライト用の構造体
struct PointLight
{
    float4 color; // 光の色
    float3 position; // 光の位置
    float intensity; // 光の強度
    float radius; // 光の半径
    float decay; // 光の減衰率
    float2 padding; // パディング
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<CameraInfo> gCamera : register(b3);
// ★追加：ポイントライトの定数バッファをレジスタb4に割り当て
ConstantBuffer<PointLight> gPointLight : register(b4);

struct PixelShaderOutput
{
    float4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * gMaterial.color;
    
    return output;
}
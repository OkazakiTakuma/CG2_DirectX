#include "Particle.hlsli"

// テクスチャ (t0)
// ※ここはグローバル領域（波括弧の外）になければなりません
Texture2D<float4> gTexture : register(t0);

// サンプラー (s0)
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 頂点色とテクスチャ色を乗算
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * input.color;
    
    // アルファテスト（透明なら描画しない）
    if (output.color.a == 0.0)
    {
        discard;
    }

    return output;
}
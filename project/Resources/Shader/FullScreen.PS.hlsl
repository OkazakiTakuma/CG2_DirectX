#include "FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 💡【変更】定数バッファにフラグを追加
cbuffer ColorInfo : register(b0)
{
    float4 tintColor;
    int enableGrayscale; // 1ならON、0ならOFF
};

struct PixelShaderOutPut
{
    float4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    PixelShaderOutPut output;
    
    float4 baseColor = gTexture.Sample(gSampler, input.uv);
    
    // 💡【変更】フラグによって処理を分ける
    if (enableGrayscale != 0)
    {
        // ONの場合：グレースケールにしてから色(tintColor)を掛ける
        float gray = dot(baseColor.rgb, float3(0.299f, 0.587f, 0.114f));
        output.color = float4(gray, gray, gray, baseColor.a) * tintColor;
    }
    else
    {
        // OFFの場合：元の色にそのまま色(tintColor)を掛ける
        output.color = baseColor * tintColor;
    }
    
    return output;
}
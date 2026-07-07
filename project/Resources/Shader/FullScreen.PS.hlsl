#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer ColorInfo : register(b0)
{
    float4 tintColor;
    int enableGrayscale;
    int enableVignetting;
    float vignetteIntensity;
    float vignetteRadius;
    float vignetteSoftness;
};

struct PixelShaderOutPut
{
    float4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutput input)
{
    PixelShaderOutPut output;

    float4 baseColor = gTexture.Sample(gSampler, input.uv);
    float4 resultColor = baseColor;

    if (enableGrayscale != 0)
    {
        float gray = dot(baseColor.rgb, float3(0.299f, 0.587f, 0.114f));
        resultColor = float4(gray, gray, gray, baseColor.a);
    }

    if (enableVignetting != 0)
    {
        float2 centeredUv = input.uv - float2(0.5f, 0.5f);
        float distanceFromCenter = length(centeredUv);
        float edgeFactor = smoothstep(vignetteRadius, vignetteRadius + vignetteSoftness, distanceFromCenter);
        float vignette = 1.0f - saturate(edgeFactor * vignetteIntensity);
        resultColor.rgb *= vignette;
    }

    output.color = resultColor * tintColor;
    return output;
}

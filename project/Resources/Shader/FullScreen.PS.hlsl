#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer ColorInfo : register(b0)
{
    float4 tintColor;
    int enableGrayscale;
    int enableVignetting;
    int enableSmoothing;
    int enableGaussianFilter;
    int enableRadialBlur;
    int enableRandom;
    int radialBlurSamples;
    float vignetteIntensity;
    float vignetteRadius;
    float vignetteSoftness;
    float radialBlurStrength;
    float randomStrength;
    float time;
    float2 texelSize;
    float padding;
};

struct PixelShaderOutPut
{
    float4 color : SV_TARGET0;
};

float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f)) + time * 43.758f) * 43758.5453f);
}

float4 ApplySmoothing(float2 uv)
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            color += gTexture.Sample(gSampler, uv + float2(x, y) * texelSize);
        }
    }

    return color / 9.0f;
}

float4 ApplyGaussianFilter(float2 uv)
{
    static const float kernel[9] = {
        1.0f, 2.0f, 1.0f,
        2.0f, 4.0f, 2.0f,
        1.0f, 2.0f, 1.0f
    };

    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int kernelIndex = 0;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            color += gTexture.Sample(gSampler, uv + float2(x, y) * texelSize) * kernel[kernelIndex];
            ++kernelIndex;
        }
    }

    return color / 16.0f;
}

float4 ApplyRadialBlur(float2 uv)
{
    const float2 center = float2(0.5f, 0.5f);
    const float2 direction = uv - center;
    const int sampleCount = clamp(radialBlurSamples, 2, 32);
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int index = 0; index < sampleCount; ++index)
    {
        const float rate = (float)index / (float)(sampleCount - 1);
        const float2 sampleUv = uv - direction * radialBlurStrength * rate;
        color += gTexture.Sample(gSampler, sampleUv);
    }

    return color / (float)sampleCount;
}

PixelShaderOutPut main(VertexShaderOutput input)
{
    PixelShaderOutPut output;

    float4 baseColor = gTexture.Sample(gSampler, input.uv);
    float4 resultColor = baseColor;

    if (enableSmoothing != 0)
    {
        resultColor = ApplySmoothing(input.uv);
    }

    if (enableGaussianFilter != 0)
    {
        resultColor = ApplyGaussianFilter(input.uv);
    }

    if (enableRadialBlur != 0)
    {
        resultColor = ApplyRadialBlur(input.uv);
    }

    if (enableGrayscale != 0)
    {
        float gray = dot(resultColor.rgb, float3(0.299f, 0.587f, 0.114f));
        resultColor = float4(gray, gray, gray, resultColor.a);
    }

    if (enableVignetting != 0)
    {
        float2 centeredUv = input.uv - float2(0.5f, 0.5f);
        float distanceFromCenter = length(centeredUv);
        float edgeFactor = smoothstep(vignetteRadius, vignetteRadius + vignetteSoftness, distanceFromCenter);
        float vignette = 1.0f - saturate(edgeFactor * vignetteIntensity);
        resultColor.rgb *= vignette;
    }

    if (enableRandom != 0)
    {
        const float noise = Random(input.uv) * 2.0f - 1.0f;
        resultColor.rgb += noise * randomStrength;
    }

    output.color = resultColor * tintColor;
    return output;
}

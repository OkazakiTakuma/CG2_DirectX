#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
Texture2D<float> gDissolveMask : register(t2);
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
    int enableOutline;
    int enableDissolve;
    float vignetteIntensity;
    float vignetteRadius;
    float vignetteSoftness;
    float radialBlurStrength;
    float randomStrength;
    float outlineStrength;
    float outlineThreshold;
    float outlineThickness;
    float dissolveThreshold;
    float dissolveEdgeWidth;
    float time;
    float2 texelSize;
    float2 paddingTexel;
    float4 outlineColor;
    float4 dissolveEdgeColor;
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

float4 ApplyDepthOutline(float2 uv, float4 sourceColor)
{
    const float2 offset = texelSize * outlineThickness;
    const float centerDepth = gDepthTexture.Sample(gSampler, uv);
    float maxDepthDifference = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }

            const float sampleDepth = gDepthTexture.Sample(gSampler, uv + float2(x, y) * offset);
            maxDepthDifference = max(maxDepthDifference, abs(centerDepth - sampleDepth));
        }
    }

    const float edge = smoothstep(outlineThreshold, outlineThreshold + 0.002f, maxDepthDifference) * outlineStrength;
    sourceColor.rgb = lerp(sourceColor.rgb, outlineColor.rgb, saturate(edge));
    return sourceColor;
}

float4 ApplyDissolve(float2 uv, float4 sourceColor)
{
    const float animatedMask = gDissolveMask.Sample(gSampler, uv + float2(time * 0.015f, time * 0.01f));
    clip(animatedMask - dissolveThreshold);

    const float edgeWidth = max(dissolveEdgeWidth, 0.0001f);
    const float edge = 1.0f - smoothstep(dissolveThreshold, dissolveThreshold + edgeWidth, animatedMask);
    sourceColor.rgb = lerp(sourceColor.rgb, dissolveEdgeColor.rgb, saturate(edge) * dissolveEdgeColor.a);
    return sourceColor;
}

PixelShaderOutPut main(VertexShaderOutput input)
{
    PixelShaderOutPut output;

    float4 resultColor = gTexture.Sample(gSampler, input.uv);

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

    if (enableOutline != 0)
    {
        resultColor = ApplyDepthOutline(input.uv, resultColor);
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

    if (enableDissolve != 0)
    {
        resultColor = ApplyDissolve(input.uv, resultColor);
    }

    output.color = resultColor * tintColor;
    return output;
}

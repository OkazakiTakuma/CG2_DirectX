#include "CopyImage.hlsli"

Texture2D<float4> gSourceTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BloomConstants : register(b0)
{
    float threshold;
    float knee;
    float radius;
    float padding;
    float2 texelSize;
    int mode;
    float padding2;
};

float3 ExtractBright(float3 color)
{
    const float brightness = max(color.r, max(color.g, color.b));
    // Soft kneeにより閾値付近を連続的に抽出し、Bloom境界のちらつきを抑えます。
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0f, 2.0f * knee);
    soft = soft * soft / max(4.0f * knee, 0.0001f);
    const float contribution = max(brightness - threshold, soft) / max(brightness, 0.0001f);
    return color * max(contribution, 0.0f);
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    if (mode == 0)
    {
        // 4点平均で高周波ノイズを抑えながら半解像度へ縮小します。
        const float2 offset = texelSize * 0.5f;
        float3 color = 0.0f;
        color += gSourceTexture.Sample(gSampler, input.uv + float2(-offset.x, -offset.y)).rgb;
        color += gSourceTexture.Sample(gSampler, input.uv + float2( offset.x, -offset.y)).rgb;
        color += gSourceTexture.Sample(gSampler, input.uv + float2(-offset.x,  offset.y)).rgb;
        color += gSourceTexture.Sample(gSampler, input.uv + float2( offset.x,  offset.y)).rgb;
        return float4(ExtractBright(color * 0.25f), 1.0f);
    }

    const float2 direction = mode == 1 ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f);
    const float weights[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
    float3 result = gSourceTexture.Sample(gSampler, input.uv).rgb * weights[0];
    [unroll]
    for (int index = 1; index < 5; ++index)
    {
        const float2 sampleOffset = direction * texelSize * float(index) * max(radius, 0.5f);
        result += gSourceTexture.Sample(gSampler, input.uv + sampleOffset).rgb * weights[index];
        result += gSourceTexture.Sample(gSampler, input.uv - sampleOffset).rgb * weights[index];
    }
    return float4(result, 1.0f);
}

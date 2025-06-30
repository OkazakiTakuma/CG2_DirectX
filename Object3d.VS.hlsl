cbuffer WVP : register(b0)
{
    matrix wvpMatrix;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, wvpMatrix); // ← 行列を使って変換
    return output;
}

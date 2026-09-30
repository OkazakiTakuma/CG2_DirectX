struct Transform
{
    float4x4 WVP;
};

ConstantBuffer<Transform> gTransform : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 viewDirection : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // zをwと同じ値にし、通常の3Dオブジェクトより奥へSkyを固定します。
    output.position = mul(input.position, gTransform.WVP).xyww;
    // 立方体のローカル方向はワールドの空を見る方向としてそのまま利用できます。
    output.viewDirection = input.position.xyz;
    return output;
}

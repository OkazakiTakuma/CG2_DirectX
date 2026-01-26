#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    // --- 追加: 法線変換用の逆転置行列 ---
    float4x4 worldInverseTranspose;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    
    // --- 変更: 法線の変換に worldInverseTranspose を使用 ---
    // 非均一スケールに対応するため、ワールド行列そのものではなく、逆転置行列を使用する
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.worldInverseTranspose));
    
    output.worldPosition = mul(input.position, gTransformationMatrix.world).xyz;
    return output;
}
#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 WorldInverseTranspose;
};

// ★変更点1：ConstantBuffer から StructuredBuffer（構造化バッファの配列）に変更！
// ピクセルシェーダーで t0(テクスチャ), t1(環境マップ) を使っているため、ここは t2 にします。
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t2);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    // ★変更点2：GPUから「自分が配列の何番目か」を教えてもらう変数を追加
    uint instanceId : SV_InstanceID;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // ★変更点3：自分のIDを使って、配列から自分専用の行列データを取り出す
    TransformationMatrix mat = gTransformationMatrices[input.instanceId];
    
    // 以下、gTransformationMatrix を取り出した mat に置き換えます
    output.position = mul(input.position, mat.WVP);
    output.texcoord = input.texcoord;
    
    output.normal = normalize(mul(input.normal, (float3x3) mat.WorldInverseTranspose));
    output.worldPosition = mul(input.position, mat.world).xyz;
    
    return output;
}
#include "Object3d.hlsli"

// カメラやオブジェクトの行列
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

// ボーンの行列パレット
struct SkinCluster
{
    float4x4 bones[256];
};
ConstantBuffer<SkinCluster> gSkinCluster : register(b2); 

// スキンメッシュ用の頂点入力
struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // ボーンのウェイトを使って、この頂点の「最終的な変形行列」を作る
    float4x4 skinMatrix = 
        gSkinCluster.bones[input.boneIndices.x] * input.boneWeights.x +
        gSkinCluster.bones[input.boneIndices.y] * input.boneWeights.y +
        gSkinCluster.bones[input.boneIndices.z] * input.boneWeights.z +
        gSkinCluster.bones[input.boneIndices.w] * input.boneWeights.w;

    // 頂点の位置と法線をスキンマトリックスで動かす
    float4 skinnedPosition = mul(input.position, skinMatrix);
    float3 skinnedNormal = mul(input.normal, (float3x3)skinMatrix);

    output.position = mul(skinnedPosition, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(skinnedNormal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(skinnedPosition, gTransformationMatrix.world).xyz;
    
    return output;
}
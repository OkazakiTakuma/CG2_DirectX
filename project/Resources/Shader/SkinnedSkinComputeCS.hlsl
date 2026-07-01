#include "Object3d.hlsli"

// インプット: 元頂点バッファ（position float3, texcoord, normal, boneIndices, boneWeights）をSRVとして読み
// ボーン行列をCBVとして読み
// アウトプット: スキン済み頂点（position float3, texcoord, normal）をUAVとして書き出す

struct InVertex {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct OutVertex {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

// ボーン行列のCBV（既存のSkinClusterと整合）
cbuffer SkinCluster : register(b2) {
    float4x4 bones[256];
};

// SRV: 元頂点バッファ（StructuredBuffer）
StructuredBuffer<InVertex> gInVertices : register(t0);
// UAV: 出力頂点バッファ
RWStructuredBuffer<OutVertex> gOutVertices : register(u0);

[numthreads(64,1,1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint idx = dispatchThreadId.x;
    // 安全対策: バッファの長さチェックはアプリ側で行う想定
    InVertex v = gInVertices[idx];

    float4x4 skinMatrix =
        bones[v.boneIndices.x] * v.boneWeights.x +
        bones[v.boneIndices.y] * v.boneWeights.y +
        bones[v.boneIndices.z] * v.boneWeights.z +
        bones[v.boneIndices.w] * v.boneWeights.w;

    float4 pos4 = float4(v.position, 1.0f);
    float4 skinnedPos4 = mul(pos4, skinMatrix);
    float3 skinnedNormal = mul(v.normal, (float3x3)skinMatrix);

    OutVertex outv;
    outv.position = skinnedPos4.xyz;
    outv.texcoord = v.texcoord;
    outv.normal = normalize(skinnedNormal);

    gOutVertices[idx] = outv;
}

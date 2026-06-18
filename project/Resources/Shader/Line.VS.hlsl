struct VSInput {
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

// 定数バッファ（カメラのビュープロジェクション行列）
cbuffer cb0 : register(b0) {
    matrix viewProjection;
};

VSOutput main(VSInput input) {
    VSOutput output;
    // 座標に行列を掛けて画面上の位置に変換
    output.pos = mul(float4(input.pos, 1.0f), viewProjection);
    output.color = input.color;
    return output;
}
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

/// <summary>
/// シェーダーのメイン処理を実行し、出力値を計算します。
/// </summary>
/// <param name="input">シェーダーまたは処理に渡される入力値を指定します。</param>
/// <returns>処理結果を返します。</returns>
VSOutput main(VSInput input) {
    VSOutput output;
    // 座標に行列を掛けて画面上の位置に変換
    output.pos = mul(float4(input.pos, 1.0f), viewProjection);
    output.color = input.color;
    return output;
}

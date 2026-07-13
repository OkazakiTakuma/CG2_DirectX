struct VSOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

/// <summary>
/// シェーダーのメイン処理を実行し、出力値を計算します。
/// </summary>
/// <param name="input">シェーダーまたは処理に渡される入力値を指定します。</param>
/// <returns>処理結果を返します。</returns>
float4 main(VSOutput input) : SV_TARGET {
    return input.color; // 頂点の色をそのまま出力
}

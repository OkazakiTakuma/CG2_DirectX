#include "ModelCommon.h"
#include "../../base/DirectXCommon.h"
#include <cassert>

using namespace Logger;
/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <param name="dxCommon">DirectX 共通処理へアクセスするための参照を指定します。</param>
void ModelCommon::Initialize(DirectXCommon* dxCommon) {
	this->dxCommon_ = dxCommon; }

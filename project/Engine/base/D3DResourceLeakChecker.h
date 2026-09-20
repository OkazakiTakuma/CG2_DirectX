#pragma once
/// エンジン終了時に解放されていないD3D12/DXGIオブジェクトを報告します。
class D3DResourceLeakChecker {
public:
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~D3DResourceLeakChecker();
};

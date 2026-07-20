#pragma once
/// Reports live D3D12/DXGI objects when the engine shuts down.
class D3DResourceLeakChecker {
public:
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~D3DResourceLeakChecker();
};

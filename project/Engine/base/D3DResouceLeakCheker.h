#pragma once
class D3DResourceLeakCheker {
public:
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~D3DResourceLeakCheker();
};

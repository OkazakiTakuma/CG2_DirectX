#pragma once
#include"BaseScene.h"
#include<string>

class AbstractSceneFactory {
public:

	virtual ~AbstractSceneFactory() = default;

	/// <summary>
	/// Scene を作成し、利用できる状態にします。
	/// </summary>
	/// <param name="sceneName">対象となるシーン名を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};

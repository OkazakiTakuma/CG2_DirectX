#include "SceneFactory.h"
#include <TitleScene.h>
#include <GamePlayScene.h>

/// <summary>
/// Scene を作成し、利用できる状態にします。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
/// <returns>処理結果を返します。</returns>
std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) { 
	if (sceneName == "TITLE") {
		return std::make_unique<TitleScene>();
	}
	if (sceneName == "GAMEPLAY") {
		return std::make_unique<GamePlayScene>();
	}

	return nullptr;
}


#include "SceneFactory.h"
#include <TitleScene.h>
#include <GamePlayScene.h>
#include <PlayerSelectScene.h>

/// <summary>
/// Scene を作成し、利用できる状態にします。
/// </summary>
/// <param name="sceneName">対象となるシーン名を指定します。</param>
std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) { 
	if (sceneName == "TITLE") {
		return std::make_unique<TitleScene>();
	}
	if (sceneName == "GAMEPLAY") {
		return std::make_unique<GamePlayScene>();
	}
	if (sceneName == "PLAYER_SELECT") {
		return std::make_unique<PlayerSelectScene>();
	}

	return nullptr;
}


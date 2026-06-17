#include "SceneFactory.h"
#include <TitleScene.h>
#include <GamePlayScene.h>

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) { 
	if (sceneName == "TITLE") {
		return std::make_unique<TitleScene>();
	}
	if (sceneName == "GAMEPLAY") {
		return std::make_unique<GamePlayScene>();
	}

	return nullptr;
}


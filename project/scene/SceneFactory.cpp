#include "SceneFactory.h"
#include "SceneFactory.h"
#include <TitleScene.h>
#include <GamePlayScene.h>

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) { 
	std::unique_ptr<BaseScene> scene = nullptr;
	if (sceneName == "TITLE") {
		scene = std::make_unique<TitleScene>();
	} else if (sceneName == "GAMEPLAY") {
		scene = std::make_unique<GamePlayScene>();
	}

	return scene;
}


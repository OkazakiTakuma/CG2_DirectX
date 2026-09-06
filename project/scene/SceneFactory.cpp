#include "SceneFactory.h"
#include <TitleScene.h>
#include <GamePlayScene.h>
#include <PlayerSelectScene.h>
#include <StageSelectScene.h>
#include <ResultScene.h>
#include <ShopScene.h>

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
	if (sceneName == "STAGE_SELECT") {
		return std::make_unique<StageSelectScene>();
	}
	// ゲームプレイ終了時に保存された戦績を表示するリザルトシーンを生成する。
	if (sceneName == "RESULT") {
		return std::make_unique<ResultScene>();
	}
	// タイトルメニューから指定されるショップ専用シーンを生成する。
	if (sceneName == "SHOP") {
		return std::make_unique<ShopScene>();
	}

	return nullptr;
}


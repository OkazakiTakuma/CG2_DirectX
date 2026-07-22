#include "FlameWork.h"
#include "Game.h"
#include <memory>

/// <summary>
/// アプリケーションのエントリーポイントとしてゲームを起動し、終了まで実行します。
/// </summary>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	std::unique_ptr<FlameWork> game = std::make_unique<Game>();
	game->Run();

	return 0;
}

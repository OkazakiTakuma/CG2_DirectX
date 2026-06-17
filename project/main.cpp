#include "FlameWork.h"
#include "Game.h"
#include <memory>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	std::unique_ptr<FlameWork> game = std::make_unique<Game>();
	game->Run();

	return 0;
}

#include "FlameWork.h"
#include "Game.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	FlameWork* game = new Game();
	game->Run();
	delete game;

	return 0;
}

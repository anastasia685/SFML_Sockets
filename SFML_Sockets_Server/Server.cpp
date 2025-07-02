#include <iostream>

#include "Game.h"
using namespace Server;

int main()
{
	try
	{
		Game game;
		game.run();
	}
	catch (std::exception& e)
	{
		std::cout << "EXCEPTION: " << e.what() << std::endl;
	}

	return 0;
}
#pragma once
#include <iostream>
#include "World.h"
#include "NetworkManager.h"
#include "Event.h"
#include "Player.h"
#include "Bullet.h"

class Game
{
public:
	Game();
	void run();

private:
	const int FrameRate = 60;

private:
	void processEvents(sf::Time dt);
	void update(sf::Time dt);
	void render();

	void handlePlayerInput(sf::Time dt);

private:
	sf::RenderWindow mWindow;
	World mWorld;
	NetworkManager mNetworkManager;
	Player* mPlayer;
	bool mIsPaused;
};

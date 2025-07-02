#pragma once

#define MAX_MOVES 60
#define MAX_SHOTS 10

struct PlayerMessage
{
	sf::Uint32 type;
	sf::Uint32 id;
	float x, y;
	float vx, vy;
	float dt;
	uint32_t timestamp, lastUpdate;
	uint32_t startMove, endMove;
	std::bitset<MAX_MOVES> movesBitmap;
	uint32_t startShot, endShot;
	std::bitset<MAX_SHOTS> shotsBitmap;
	sf::Uint32 color;

	sf::Uint32 playerId;
	int score;
};

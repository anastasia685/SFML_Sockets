#pragma once
#include "Entity.h"

class Bullet : public Entity
{
public:
	static const float Range;
	static const float Speed;
	static const uint32_t ShotRate;


	Bullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction, bool hostile);

	unsigned int getPlayerId() const;
	uint32_t getTimestamp() const;

private:
	// override scenenode's drawcurrent function
	virtual void drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;

	// override scenenode's event handling
	virtual unsigned int getCategory() const;

	virtual void updateCurrent(sf::Time dt, unsigned short _playerId);

private:
	sf::RectangleShape mShape;
	unsigned int mPlayerId;
	uint32_t mTimestamp;
	sf::Vector2f mSpawnPosition;
};

#include "Bullet.h"

const float Bullet::Range = 250.f;
const float Bullet::Speed = 350.f;
const uint32_t Bullet::ShotRate = 200; // in millis

Bullet::Bullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction, bool hostile) : mPlayerId(playerId), mTimestamp(timestamp), mSpawnPosition(spawnPosition), mShape(sf::Vector2f(8.f, 8.f))
{
	setPosition(mSpawnPosition);

	sf::FloatRect bounds = mShape.getLocalBounds();
	mShape.setOrigin(bounds.width / 2.f, bounds.height / 2.f);

	mShape.setFillColor(hostile ? sf::Color::Red : sf::Color::White);

	setVelocity(direction * Speed);
}
unsigned int Bullet::getPlayerId() const
{
	return mPlayerId;
}
uint32_t Bullet::getTimestamp() const
{
	return mTimestamp;
}
void Bullet::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	// call rendertarget(window) draw function with this class' drawable (rectshape)
	target.draw(mShape, states);
}

unsigned int Bullet::getCategory() const
{
	return SceneNode::Type::Bullet;
}

void Bullet::updateCurrent(sf::Time dt, unsigned short _playerId)
{
	// TODO: clamp to max range
	sf::Vector2f newPos = getPosition() + getVelocity() * dt.asSeconds();
	setPosition(newPos);

	
	float dist = sqrt(pow(newPos.x - mSpawnPosition.x, 2) + pow(newPos.y - mSpawnPosition.y, 2));
	if (dist >= Range)
	{
		flagForRemoval();
	}
}

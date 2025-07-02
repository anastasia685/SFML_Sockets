#include "Bullet.h"
using namespace Server;

const float Server::Bullet::Range = 250.f;
const float Server::Bullet::Speed = 350.f;
const uint32_t Bullet::ShotRate = 200; // in millis

Server::Bullet::Bullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction) : mPlayerId(playerId), mTimestamp(timestamp), mSpawnPosition(spawnPosition), mDirection(direction)
{
}

sf::Vector2f Server::Bullet::getSpawnPosition() const
{
	return mSpawnPosition;
}

sf::Vector2f Server::Bullet::getDirection() const
{
	return mDirection;
}
unsigned int Server::Bullet::getPlayerId() const 
{
	return mPlayerId;
};
uint32_t Server::Bullet::getTimestamp() const
{
	return mTimestamp;
}

sf::Vector2f Server::Bullet::getPosition() const
{
    return mPosition;
}

void Server::Bullet::setPosition(sf::Vector2f position)
{
    mPosition = position;
}

void Server::Bullet::update(sf::Time dt)
{
    mPosition += mDirection * Speed * dt.asSeconds();
}

sf::Vector2f Server::Bullet::simulateMovement(sf::Time dt)
{
	return mSpawnPosition + mDirection * Speed * dt.asSeconds();
}

bool Server::Bullet::checkCollision(sf::Vector2f bulletPos, sf::Vector2f playerPos)
{
    float bulletSize = 8, playerSize = 30;

    float halfBulletSize = bulletSize / 2.0f;
    float halfPlayerSize = playerSize / 2.0f;

    // Calculate bounding box ranges for the bullet
    float bulletLeft = bulletPos.x - halfBulletSize;
    float bulletRight = bulletPos.x + halfBulletSize;
    float bulletTop = bulletPos.y - halfBulletSize;
    float bulletBottom = bulletPos.y + halfBulletSize;

    // Calculate bounding box ranges for the player
    float playerLeft = playerPos.x - halfPlayerSize;
    float playerRight = playerPos.x + halfPlayerSize;
    float playerTop = playerPos.y - halfPlayerSize;
    float playerBottom = playerPos.y + halfPlayerSize;

    // Check for overlap
    bool horizontalOverlap = bulletRight > playerLeft && bulletLeft < playerRight;
    bool verticalOverlap = bulletBottom > playerTop && bulletTop < playerBottom;

    return horizontalOverlap && verticalOverlap;
}

bool Server::Bullet::hasHitPlayer(unsigned int playerId) const
{
    return mHitPlayers.find(playerId) != mHitPlayers.end();
}
void Server::Bullet::markPlayerAsHit(unsigned int playerId)
{
    mHitPlayers.insert(playerId);
}
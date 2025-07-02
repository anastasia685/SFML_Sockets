#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_set>

namespace Server
{
	class Bullet
	{
	public:
		static const float Range;
		static const float Speed;
		static const uint32_t ShotRate;
		static bool checkCollision(sf::Vector2f bulletPos, sf::Vector2f playerPos);

		Bullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction);
		sf::Vector2f getSpawnPosition() const;
		sf::Vector2f getDirection() const;
		unsigned int getPlayerId() const;
		uint32_t getTimestamp() const;
		sf::Vector2f getPosition() const;
		void setPosition(sf::Vector2f position);

		void update(sf::Time dt);
		sf::Vector2f simulateMovement(sf::Time dt);


		bool hasHitPlayer(unsigned int playerId) const;
		void markPlayerAsHit(unsigned int playerId);

	private:
		unsigned short mPlayerId;
		uint32_t mTimestamp;
		sf::Vector2f mSpawnPosition;
		sf::Vector2f mDirection;

		sf::Vector2f mPosition;

		std::unordered_set<unsigned int> mHitPlayers;
	};
}
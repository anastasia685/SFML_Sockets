#pragma once
#include <vector>
#include "Player.h"
#include "Bullet.h"
#include "NetworkManager.h"

namespace Server
{
	typedef std::unique_ptr<Player> PlayerPtr;
	typedef std::unique_ptr<Bullet> BulletPtr;
	class Game
	{
	public:
		Game();
		void run();
		unsigned short addPlayer();
		void removePlayer(unsigned short id);
		Player* getPlayer(unsigned short id);
		const std::vector<PlayerPtr>& getPlayers() const;

		void addBullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction, uint32_t now);
		void addBullet(std::unique_ptr<Bullet> newBullet, uint32_t now);
		std::vector<BulletPtr>::iterator removeBullet(unsigned int playerId, uint32_t timestamp);
		const std::vector<BulletPtr>& getBullets() const;

	private:
		void update(sf::Time dt);
	private:
		std::vector<PlayerPtr> mPlayers;
		std::vector<BulletPtr> mBullets;
		NetworkManager mNetworkManager;
	};
}
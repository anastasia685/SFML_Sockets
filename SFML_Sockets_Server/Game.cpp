#include "Game.h"
using namespace Server;

Server::Game::Game() : mPlayers(), mNetworkManager(this) {}

void Server::Game::run()
{
	sf::Clock clock;
	sf::Time timeSinceLastUpdate;
	sf::Time dt = sf::seconds(1.f / 60.f);

	std::cout << "Server listening at: " << sf::IpAddress::getLocalAddress().toString() << ":" << SERVER_PORT << std::endl;
	while (true)
	{
		timeSinceLastUpdate += clock.restart();
		while (timeSinceLastUpdate >= dt)
		{
			timeSinceLastUpdate -= dt;

			// receive & process updates
			mNetworkManager.update(dt);


			auto now = std::chrono::high_resolution_clock::now();
			auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			uint32_t timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);

			for (auto it = mBullets.begin(); it != mBullets.end();)
			{
				(*it)->update(dt);

				if ((sf::milliseconds((*it)->getTimestamp()) + sf::seconds(Bullet::Range / Bullet::Speed)).asMilliseconds() < timestamp)
				{
					it = removeBullet((*it)->getPlayerId(), (*it)->getTimestamp());
				}
				else
				{
					it++;
				}
			}
			

			mNetworkManager.send();
		}
	}
}

unsigned short Server::Game::addPlayer()
{
	std::unique_ptr<Player> player(new Player());
	unsigned short id = player.get()->getId();
	mPlayers.push_back(std::move(player));
	return id;
}

void Server::Game::removePlayer(unsigned short id)
{
	auto found = std::find_if(mPlayers.begin(), mPlayers.end(), [id](PlayerPtr& p) -> bool { return p.get()->getId() == id; });

	if (found != mPlayers.end())
	{
		mPlayers.erase(found);
	}
}

Player* Server::Game::getPlayer(unsigned short id)
{
	auto found = std::find_if(mPlayers.begin(), mPlayers.end(), [id](PlayerPtr& p) -> bool { return p.get()->getId() == id; });
	if (found == mPlayers.end()) return nullptr;
	return found->get();
}

const std::vector<PlayerPtr>& Server::Game::getPlayers() const
{
	return mPlayers;
}

void Server::Game::addBullet(unsigned int playerId, uint32_t timestamp, sf::Vector2f spawnPosition, sf::Vector2f direction, uint32_t now)
{
	std::unique_ptr<Bullet> bullet(new Bullet(playerId, timestamp, spawnPosition, direction));
	bullet->setPosition(spawnPosition + direction * Bullet::Speed * sf::milliseconds(now - timestamp).asSeconds());
	mBullets.push_back(std::move(bullet));
}
void Server::Game::addBullet(std::unique_ptr<Bullet> newBullet, uint32_t now)
{
	newBullet->setPosition(newBullet->getSpawnPosition() + newBullet->getDirection() * Bullet::Speed * sf::milliseconds(now - newBullet->getTimestamp()).asSeconds());
	mBullets.push_back(std::move(newBullet));
}

std::vector<BulletPtr>::iterator Server::Game::removeBullet(unsigned int playerId, uint32_t timestamp)
{
	auto found = std::find_if(mBullets.begin(), mBullets.end(), [playerId, timestamp](BulletPtr& b) -> bool { return b.get()->getPlayerId() == playerId && b.get()->getTimestamp() == timestamp; });

	if (found != mBullets.end())
	{
		return mBullets.erase(found);
	}

	return mBullets.end();
}

const std::vector<BulletPtr>& Server::Game::getBullets() const
{
	return mBullets;
}

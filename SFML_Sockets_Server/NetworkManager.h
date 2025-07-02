#pragma once
#include <iostream>
#include <SFML/Network.hpp>
#include <map>
#include <queue>
#include <deque>
#include <vector>
#include <bitset>
#include <chrono>
#include "Bullet.h"

namespace Server
{
	const int SERVER_PORT = 5555;
	const size_t MAX_MOVES = 60;
	const size_t MAX_SHOTS = 10;
	class Game;

	class NetworkManager
	{
	public:
		NetworkManager(Game* game);

		void update(sf::Time dt);
		void send();
		void receive();

	private:
		enum PacketType
		{
			Hello = 0,
			Welcome = 1,
			Remove = 2,
			PlayerState = 3,
			BulletsState = 4,
			PlayerUpdate = 5,
		};
		enum UpdateType
		{
			Move = 0,
			Shot = 1,
		};
		struct Message
		{
			sf::Uint32 type;
			sf::Uint32 objectID; // the ID number of the game object
			float x, y; // object position
			float vx, vy;
			float dt;
			uint32_t timestamp; // move start timestamp
			sf::Uint32 color;
		};

		struct PlayerMessage
		{
			UpdateType type;
			unsigned int playerID;
			sf::Uint32 objectID;
			sf::Vector2f force;
			float dt;
			uint32_t timestamp;
			sf::Vector2f position;
		};

	private:
		Game* mGame;
		sf::UdpSocket mSocket;
		sf::SocketSelector mSelector;
		std::map<std::pair<sf::IpAddress, unsigned short>, unsigned short> mClients; // (ip:port, playerId) map
		sf::Time mTime;

		//std::queue<NetUpdate> mMessageQueue;
		std::deque<PlayerMessage> mMessageQueue;

		void bufferMessage(const PlayerMessage& update);
		PlayerMessage processMessage();
		bool hasMessages() const;
	};

	sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_MOVES>& bitmap);
	sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_MOVES>& bitmap);

	sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_SHOTS>& bitmap);
	sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_SHOTS>& bitmap);
}
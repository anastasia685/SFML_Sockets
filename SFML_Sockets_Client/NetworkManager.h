#pragma once
#include <SFML/Network.hpp>
#include <deque>
#include <bitset>
#include "PlayerMessage.h"
#include "Player.h"
#include "Bullet.h"
#include "EventQueue.h"

//const std::string SERVER_IP = "127.0.0.1";
//const int SERVER_PORT = 5555;

class NetworkManager
{
public:
	NetworkManager();

	void update(Player** player, SceneNode& rootPlayerNode, SceneNode& rootBulletNode, EventQueue& events, sf::Time dt);
	void send(Player* player);
	void receive(Player** player, SceneNode& rootPlayerNode, SceneNode& rootBulletNode);

	void bufferMessage(const PlayerMessage& msg);
	PlayerMessage processMessage();
	bool hasMessages() const;

	void setServerIp(std::string ip);
	void setServerPort(int port);

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

private:
	std::string SERVER_IP;
	int SERVER_PORT;

	sf::UdpSocket mSocket;
	sf::SocketSelector mSelector;
	std::deque<PlayerMessage> mMessageQueue;
	sf::Time mTimeSend;
	sf::Time mTimeRead;
};

sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_MOVES>& bitmap);
sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_MOVES>& bitmap);

sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_SHOTS>& bitmap);
sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_SHOTS>& bitmap);
#pragma once
#include "Player.h"
#include "EventQueue.h"

class World : private sf::NonCopyable
{
public:
	typedef std::unique_ptr<Player> PlayerPtr;

	World(sf::RenderWindow& window);
	void update(sf::Time dt, unsigned short playerId);
	void draw();
	EventQueue& getEventQueue();

	SceneNode& getRootSceneNode();
	SceneNode& getRootPlayerNode();
	SceneNode& getRootBulletNode();

private:
	void buildScene();

private:
	sf::RenderWindow& mWindow;
	sf::View mView;
	SceneNode mSceneGraph; // root for scene node hierarchy
	EventQueue mEventQueue;

	sf::Vector2f mSpawnPosition;
	//Player* mPlayer;
	std::map<int, PlayerPtr> mPlayers;
};

#include "World.h"

World::World(sf::RenderWindow& window)
	: mWindow(window)
	, mView(window.getDefaultView())
	, mSceneGraph()
	, mSpawnPosition(mView.getSize().x / 2.f, mView.getSize().y / 2.f)
	, mPlayers()
{
	buildScene();

	// redundant for default view
	//mView.setCenter(mSpawnPosition);
}

void World::update(sf::Time dt, unsigned short playerId)
{
	// apply cached events
	while (!mEventQueue.isEmpty())
		mSceneGraph.onEvent(mEventQueue.pop(), dt);

	// update every world entity
	mSceneGraph.update(dt, playerId);
}

void World::draw()
{
	mWindow.setView(mView);
	mWindow.draw(mSceneGraph);
}

EventQueue& World::getEventQueue()
{
	return mEventQueue;
}

SceneNode& World::getRootSceneNode()
{
	return mSceneGraph;
}

SceneNode& World::getRootPlayerNode()
{
	const auto& children = mSceneGraph.getChildren();
	
	return *children[0];
}

SceneNode& World::getRootBulletNode()
{
	const auto& children = mSceneGraph.getChildren();

	return *children[1];
}

void World::buildScene()
{	
	//unsigned int localPlayerId = rand() % 900 + 100;
	//localPlayerId = 0;

	//const sf::Color colorArray[5] = { sf::Color::Cyan, sf::Color::Blue, sf::Color::Green, sf::Color::Red, sf::Color::Yellow };
	//srand(time(NULL));

	//std::unique_ptr<Player> player(new Player(localPlayerId, colorArray[rand() % 5]));
	//player->setPosition(mSpawnPosition);
	//mSceneGraph.addChild(std::move(player)); // move ownership to scene node graph
	//

	std::unique_ptr<SceneNode> rootPlayer(new SceneNode());
	mSceneGraph.addChild(std::move(rootPlayer));

	std::unique_ptr<SceneNode> rootBullet(new SceneNode());
	mSceneGraph.addChild(std::move(rootBullet));

	//SquareMover::initCallbacks(mEventQueue, localPlayerId);
}

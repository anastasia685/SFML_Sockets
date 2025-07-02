#include "Game.h"

Game::Game() : mWindow(sf::VideoMode(800, 600), "Client", sf::Style::Titlebar | sf::Style::Close), mWorld(mWindow), mIsPaused(false), mPlayer(nullptr)
{
	//mWindow.setFramerateLimit(FrameRate);
}

void Game::run()
{
	sf::Clock clock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;

	std::cout << "Enter the server IP address: ";
	std::string serverIp;
	std::cin >> serverIp;

	// Prompt the user for the server port
	std::cout << "Enter the server port: ";
	unsigned short serverPort;
	std::cin >> serverPort;

	mNetworkManager.setServerIp(serverIp);
	mNetworkManager.setServerPort(serverPort);

	while (mWindow.isOpen())
	{
		timeSinceLastUpdate += clock.restart(); // keep any remnants of previous tick with +=
		while (timeSinceLastUpdate >= sf::seconds(1.f / FrameRate))
		{
			timeSinceLastUpdate -= sf::seconds(1.f / FrameRate);
			processEvents(sf::seconds(1.f / FrameRate));
			update(sf::seconds(1.f / FrameRate));
			render();
		}
	}
}

void Game::processEvents(sf::Time dt)
{
	sf::Event event;
	while (mWindow.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
		{
			mWindow.close();
			break;
		}
		case sf::Event::GainedFocus:
		{
			mIsPaused = false;
			break;
		}
		case sf::Event::LostFocus:
		{
			mIsPaused = true;
			break;
		}
		}
	}
	handlePlayerInput(dt);
}

void Game::update(sf::Time dt)
{
	mNetworkManager.update(&mPlayer, mWorld.getRootPlayerNode(), mWorld.getRootBulletNode(), mWorld.getEventQueue(), dt);
	unsigned short mPlayerId = mPlayer != nullptr ? mPlayer->getId() : -1;
	mWorld.update(dt, mPlayerId);
	mNetworkManager.send(mPlayer);
}

void Game::render()
{
	mWindow.clear();
	mWorld.draw();
	mWindow.display();
}

void Game::handlePlayerInput(sf::Time dt)
{
	// don't update until connected to the server
	if (mPlayer == nullptr) return;

	auto now = std::chrono::high_resolution_clock::now();
	auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	uint32_t truncated_timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);


	sf::Vector2f movingForce(0, 0);
	EventQueue& events = mWorld.getEventQueue();

	if (!mIsPaused)
	{
		// rotate player box towards mouse cursor
		sf::Vector2i mousePosition = sf::Mouse::getPosition(mWindow);
		sf::Vector2f playerPosition = mPlayer->getPosition();
		sf::Vector2f direction(mousePosition.x - playerPosition.x, mousePosition.y - playerPosition.y);
		float theta = std::atan2(direction.y, direction.x) * 57.29577957; // radian to degree
		mPlayer->setRotation(theta);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			movingForce.y--;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			movingForce.y++;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			movingForce.x--;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			movingForce.x++;
		}
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			if (mPlayer->getShotsCache().empty() || mPlayer->getShotsCache().back().timestamp + Bullet::ShotRate < truncated_timestamp)
			{
				SceneNode& rootBulletNode = mWorld.getRootBulletNode();

				sf::Vector2f normalizedDirection;
				float magnitude = std::sqrt(direction.x * direction.x + direction.y * direction.y);
				if (magnitude != 0) {
					normalizedDirection = direction / magnitude;
				}
				else 
				{
					float theta = mPlayer->getRotation() * 0.01745329f; // degree to radians
					normalizedDirection = sf::Vector2f(std::cos(theta), std::sin(theta));
				}

				std::unique_ptr<Bullet> bullet(new Bullet(mPlayer->getId(), truncated_timestamp, mPlayer->getPosition(), sf::Vector2f(normalizedDirection.x, normalizedDirection.y), false));
				rootBulletNode.addChild(std::move(bullet));

				// add to message cache for sending
				mPlayer->updateShotsCache(truncated_timestamp, mPlayer->getPosition(), sf::Vector2f(normalizedDirection.x, normalizedDirection.y));
			}
		}
	}
	


	float F = sqrt(pow(movingForce.x, 2) + pow(movingForce.y, 2));
	if (F > 0)
	{
		movingForce = movingForce / F; // normalize
		movingForce = movingForce * 180.f; // scale
	}


	// update moves cache
	// don't write if F = 0 and last written move was 0
	// prevent 0 forces to be sent; however, server still needs to know if movement was stopped (to make velocity 0),
	// so still send 0 force if last sent was non-zero

	// if all last 3 are 0, only skip then (need last 2-3 inputs for remote player interpolation)
	bool shouldSkip = false;
	if (F == 0 && mPlayer->getMovesCache().size() >= 3)
	{
		bool allZeroes = true;
		auto it = mPlayer->getMovesCache().rbegin();
		for (int i = 0; i < 3; i++)
		{
			if (it->force.x != 0 || it->force.y != 0)
			{
				allZeroes = false;
				break;
			}
			it++;
		}
		shouldSkip = allZeroes;
	}
	
	if (!shouldSkip)
	{
		mPlayer->updateMovesCache(movingForce, dt.asSeconds(), truncated_timestamp, mPlayer->getVelocity());
	}

	// apply new move to local simulation
	sf::Vector2f positionInc, newVelocity;
	positionInc = mPlayer->simulateMovement(movingForce, dt, mPlayer->getVelocity(), newVelocity);
	mPlayer->setVelocity(newVelocity);
	mPlayer->setPosition(mPlayer->getPosition() + positionInc);
}
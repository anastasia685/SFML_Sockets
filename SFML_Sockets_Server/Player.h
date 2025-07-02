#pragma once
#include <SFML/Graphics.hpp>
#include <deque>

namespace Server
{
	class Player
	{
	public:
		struct Move
		{
			// for sending
			sf::Vector2f force;
			float dt;

			// for caching
			uint32_t timestamp; // start of the move
			sf::Vector2f position; // position at the start of the mmove
			sf::Vector2f velocity; // velocity at the start of the move
			int score; // score at the start of the move
		};
		struct Shot
		{
			uint32_t timestamp;
			sf::Vector2f spawnPosition;
			sf::Vector2f direction;
		};
	public:
		Player();
		unsigned short getId();
		sf::Vector2f getPosition();
		void setPosition(sf::Vector2f position);
		void setPosition(float x, float y);
		sf::Vector2f getVelocity();
		void setVelocity(sf::Vector2f velocity);
		void setVelocity(float vx, float vy);
		int getScore() const;
		void setScore(int score);

		void update(sf::Vector2f force, sf::Time dt);

		void updateMovesCache(const Move& move);
		void updateMovesCache(const sf::Vector2f& force, float dt, uint32_t timestamp, const sf::Vector2f& position, const sf::Vector2f& velocity, int score);
		std::deque<Move>& getMovesCache() { return mMovesCache; };

		void updateShotsCache(const Shot& shot);
		void updateShotsCache(uint32_t timestamp, const sf::Vector2f& spawnPosition, const sf::Vector2f& direction);
		std::deque<Shot>& getShotsCache() { return mShotsCache; };

		sf::Vector2f simulateMovement(const sf::Vector2f& movementForce, sf::Time dt, const sf::Vector2f& velocity, sf::Vector2f& newVelocity);

		uint32_t mLastUpdate = -1;
	
	private:
		static const sf::Color Colors[5];
		static unsigned short PlayerIndex;
		static const float MaxSpeed;

	private:
		unsigned short mId;
		sf::Color mColor;
		sf::Vector2f mVelocity;
		sf::Vector2f mMovingForce;
		sf::Vector2f mPosition;

		sf::Vector2f mLastMovePosition;

		int mScore;
		uint32_t mLastShot;

		std::deque<Move> mMovesCache;
		std::deque<Shot> mShotsCache;
	};
}
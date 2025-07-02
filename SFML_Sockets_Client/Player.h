#pragma once
#include <deque>
#include <chrono>
#include "Entity.h"

class Player : public Entity
{
public:
	struct Move
	{
		// for sending
		sf::Vector2f force;
		float dt;

		// for caching
		uint32_t timestamp; // start of the move
		sf::Vector2f velocity; // velocity at the start of the move

		bool acked; // was move received on server
	};
	struct Shot
	{
		uint32_t timestamp;
		sf::Vector2f spawnPosition;
		sf::Vector2f direction;

		bool acked; // was shot received on server
	};
private:
	struct PositionSnapshot 
	{
		float x;
		float y;
		uint32_t time;
	};
	struct ServerCorrection
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
	};
public:
	Player(unsigned int id, sf::Color color);
	sf::Color getColor() const;
	virtual unsigned int getId() const;

	int getScore() const;
	void setScore(int score);


	void applyForce(const sf::Vector2f& force);
	void applyForce(float fx, float fy);

	void updatePositions(const sf::Vector2f& position, uint32_t timestamp);
	void updatePositions(float x, float y, uint32_t timestamp);
	void setSyncSnapshot(const sf::Vector2f& position, uint32_t timestamp);
	void setSyncSnapshot(float x, float y, uint32_t timestamp);

	void updateMovesCache(const Move& move);
	void updateMovesCache(const sf::Vector2f& force, float dt, uint32_t timestamp, const sf::Vector2f& velocity);
	std::deque<Move>& getMovesCache() { return mMovesCache; };

	void updateShotsCache(const Shot& shot);
	void updateShotsCache(uint32_t timestamp, const sf::Vector2f& spawnPosition, const sf::Vector2f& direction);
	std::deque<Shot>& getShotsCache() { return mShotsCache; };

	sf::Vector2f runInterpolation();
	sf::Vector2f simulateMovement(const sf::Vector2f& movementForce, sf::Time dt, const sf::Vector2f& velocity, sf::Vector2f& newVelocity);


	bool mReset = false;

	sf::Vector2f mServerCorrection;

private:
	// override scenenode's drawcurrent function
	virtual void drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;

	// override scenenode's event handling
	virtual unsigned int getCategory() const;

	virtual void updateCurrent(sf::Time dt, unsigned short playerId);

private:
	static const float MaxSpeed;
	static const float CorrectionPeriod;

	sf::RectangleShape mShape;
	sf::Text mScoreIndicator;
	sf::Font mScoreFont;
	sf::Color mColor;
	unsigned int mId;
	sf::Vector2f mMovementForce;
	int mScore;

	std::deque<PositionSnapshot> mPositions;
	PositionSnapshot mSyncSnapshot;

	std::deque<Move> mMovesCache;
	std::deque<Shot> mShotsCache;
};

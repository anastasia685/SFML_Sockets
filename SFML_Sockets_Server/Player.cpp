#include "Player.h"
using namespace Server;

const float Server::Player::MaxSpeed = 130.f;

const sf::Color Server::Player::Colors[5] = {sf::Color::Cyan, sf::Color::Blue, sf::Color::Green, sf::Color::Red, sf::Color::Yellow};
unsigned short Server::Player::PlayerIndex = 0;


Server::Player::Player() : mId(PlayerIndex++), mColor(Colors[mId]), mPosition(400, 300), mVelocity(0, 0), mScore(0), mLastMovePosition(mPosition) {}

unsigned short Server::Player::getId()
{
	return mId;
}

sf::Vector2f Server::Player::getPosition()
{
	return mPosition;
}
sf::Vector2f Server::Player::getVelocity()
{
    return mVelocity;
}

void Server::Player::setVelocity(sf::Vector2f velocity)
{
    mVelocity = velocity;
}

void Server::Player::setVelocity(float vx, float vy)
{
    setVelocity(sf::Vector2f(vx, vy));
}

int Server::Player::getScore() const
{
    return mScore;
}

void Server::Player::setScore(int score)
{
    mScore = score;
}

void Server::Player::setPosition(sf::Vector2f position)
{
	mPosition = position;
}
void Server::Player::setPosition(float x, float y)
{
	mPosition.x = x;
	mPosition.y = y;
}

void Server::Player::update(sf::Vector2f movementForce, sf::Time dt)
{
    sf::Vector2f positionInc, newVelocity;
    positionInc = simulateMovement(movementForce, dt, getVelocity(), newVelocity);
    mVelocity = newVelocity;
    mPosition += positionInc;
}

void Server::Player::updateMovesCache(const Move& newMove)
{
    if (mMovesCache.size() == 60) // if full drop oldest message
    {
        mMovesCache.pop_front();
    }

    //Move newMove{ force, dt, timestamp, velocity };
    auto it = std::lower_bound(
        mMovesCache.begin(),
        mMovesCache.end(),
        newMove,
        [](const Move& a, const Move& b)
        {
            return a.timestamp < b.timestamp;
        });

    // replace if present
    if (it != mMovesCache.end() && it->timestamp == newMove.timestamp)
    {
        *it = newMove;
    }
    // insert
    else
    {
        // Insert the new move at the correct position
        mMovesCache.insert(it, newMove);
    }

    //mMovesCache.push_back({ force, dt, timestamp, velocity });
    return;
}

void Server::Player::updateMovesCache(const sf::Vector2f& force, float dt, uint32_t timestamp, const sf::Vector2f& position, const sf::Vector2f& velocity, int score)
{
    updateMovesCache({ force, dt, timestamp, position, velocity, score });
}

void Server::Player::updateShotsCache(const Shot& newShot)
{
    updateShotsCache(newShot.timestamp, newShot.spawnPosition, newShot.direction);
}

void Server::Player::updateShotsCache(uint32_t timestamp, const sf::Vector2f& spawnPosition, const sf::Vector2f& direction)
{
    if (mShotsCache.size() == 10 && timestamp < mShotsCache.begin()->timestamp) // if full and new move is older than oldest cached move
    {
        return;
    }

    if (mShotsCache.size() == 10) // if full drop oldest message
    {
        mShotsCache.pop_front();
    }

    Shot newShot{ timestamp, spawnPosition, direction };
    auto it = std::lower_bound(
        mShotsCache.begin(),
        mShotsCache.end(),
        newShot,
        [](const Shot& a, const Shot& b)
        {
            return a.timestamp < b.timestamp;
        });

    // replace if present
    if (it != mShotsCache.end() && it->timestamp == newShot.timestamp)
    {
        *it = newShot;
    }
    // insert
    else
    {
        // Insert the new move at the correct position
        mShotsCache.insert(it, newShot);
    }
}

sf::Vector2f Server::Player::simulateMovement(const sf::Vector2f& movementForce, sf::Time dt, const sf::Vector2f& velocity, sf::Vector2f& newVelocity)
{
    float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    // Determine if movement force is applied
    //bool isForceApplied = (mMovementForce.x != 0 || mMovementForce.y != 0);
    bool isForceApplied = (movementForce.x != 0 || movementForce.y != 0);

    const float epsilon = 1e-5f; // A very small value for minimal velocity

    // Apply movement force and update velocity if force is applied
    if (isForceApplied) {
        // Calculate net force and apply acceleration
        //sf::Vector2f netForce = mMovementForce;
        sf::Vector2f netForce = movementForce;
        sf::Vector2f acceleration = netForce / 1.f; // Assuming mass = 1 for simplicity
        //this->accelerate(acceleration * dt.asSeconds());
        newVelocity = velocity + acceleration * dt.asSeconds();

        // Update velocity after applying acceleration
        //velocity = this->getVelocity();
        speed = sqrt(newVelocity.x * newVelocity.x + newVelocity.y * newVelocity.y);

        // Clamp the velocity to max speed
        if (speed > Player::MaxSpeed) {
            newVelocity = newVelocity / speed * Player::MaxSpeed;
            //this->setVelocity(velocity);
        }

        // Update position based on the velocity
        //setPosition(getPosition() + velocity * dt.asSeconds());
        return newVelocity * dt.asSeconds(); // position increment
    }
    else {
        // make velocity zero
        //this->setVelocity(0, 0);
        newVelocity = sf::Vector2f(0, 0);
        return sf::Vector2f(0, 0); // no position increment
    }
}

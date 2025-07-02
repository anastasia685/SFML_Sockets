#include "Player.h"
#include <iostream>

const float Player::MaxSpeed = 130.f;
const float Player::CorrectionPeriod = 0.1f;

Player::Player(unsigned int id, sf::Color color) : mId(id), mShape(sf::Vector2f(30.f, 30.f)), mColor(color), mScore(0)
{
	// set transformation origin in the middle
	sf::FloatRect bounds = mShape.getLocalBounds();
	mShape.setOrigin(bounds.width / 2.f, bounds.height / 2.f);

	mShape.setFillColor(mColor);

    mScoreFont.loadFromFile("Res/DefaultAriel.ttf");

    mScoreIndicator.setFont(mScoreFont);
    mScoreIndicator.setCharacterSize(22);
    mScoreIndicator.setFillColor(sf::Color::Green);
    mScoreIndicator.setStyle(sf::Text::Bold);
    mScoreIndicator.setString(std::to_string(mScore));
}

void Player::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	// call rendertarget(window) draw function with this class' drawable (rectshape)
	target.draw(mShape, states);

    sf::RenderStates textStates = states;
    textStates.transform = sf::Transform::Identity;
    target.draw(mScoreIndicator, textStates);
}

unsigned int Player::getCategory() const
{
	return SceneNode::Type::Player;
}
unsigned int Player::getId() const
{
	return mId;
}

void Player::applyForce(const sf::Vector2f& force)
{
	mMovementForce = mMovementForce + force;
}
void Player::applyForce(float fx, float fy)
{
	mMovementForce.x += fx;
	mMovementForce.y += fy;
}

void Player::updatePositions(const sf::Vector2f& position, uint32_t timestamp)
{
    updatePositions(position.x, position.y, timestamp);
}
void Player::updatePositions(float x, float y, uint32_t timestamp)
{
    // if full and new move is older than oldest cached move
    if (mPositions.size() == 3 && timestamp < mPositions.front().time)
    {
        return;
    }


    if (mPositions.size() == 3) // if full drop oldest message
    {
        mPositions.pop_front();
    }

    /*if (!mPositions.empty() && mPositions.back().time == timestamp) return;

    mPositions.push_back({ x, y, timestamp});*/

    PositionSnapshot newSnapshot{ x, y, timestamp };

    auto it = std::lower_bound(
        mPositions.begin(),
        mPositions.end(),
        newSnapshot,
        [](const PositionSnapshot& a, const PositionSnapshot& b)
        {
            return a.time < b.time;
        });

    mPositions.insert(it, newSnapshot);
    // replace if present
    //if (it != mPositions.end() && it->time == timestamp)
    //{
    //    *it = newSnapshot;
    //}
    //// insert
    //else
    //{
    //    // Insert the new move at the correct position
    //    mPositions.insert(it, newSnapshot);
    //}
}

void Player::setSyncSnapshot(const sf::Vector2f& position, uint32_t timestamp)
{
    setSyncSnapshot(position.x, position.y, timestamp);
}

void Player::setSyncSnapshot(float x, float y, uint32_t timestamp)
{
    mSyncSnapshot.x = x;
    mSyncSnapshot.y = y;
    mSyncSnapshot.time = timestamp;
}

void Player::updateMovesCache(const Move& move)
{
    updateMovesCache(move.force, move.dt, move.timestamp, move.velocity);
}

void Player::updateMovesCache(const sf::Vector2f& force, float dt, uint32_t timestamp, const sf::Vector2f& velocity)
{
    if (mMovesCache.size() == 60 && timestamp < mMovesCache.begin()->timestamp) // if full and new move is older than oldest cached move
    {
        return;
    }

    if (mMovesCache.size() == 60) // if full drop oldest message
    {
        mMovesCache.pop_front();
    }

    Move newMove{ force, dt, timestamp, velocity, false };
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
        mMovesCache.insert(it, newMove);
    }
}

void Player::updateShotsCache(const Shot& shot)
{
    updateShotsCache(shot.timestamp, shot.spawnPosition, shot.direction);
}

void Player::updateShotsCache(uint32_t timestamp, const sf::Vector2f& spawnPosition, const sf::Vector2f& direction)
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

sf::Vector2f Player::runInterpolation()
{
    auto size = mPositions.size();
    // these two are the same
    if (size == 0) return this->getPosition();
    if (size == 1) return { mPositions[0].x, mPositions[0].y };

    auto now = std::chrono::high_resolution_clock::now();
    auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    uint32_t gameTime = static_cast<uint32_t>(absolute_timestamp % 4294967295);


    PositionSnapshot pos0 = mPositions[size - 1];
    PositionSnapshot pos1 = mPositions[size - 2];

    //---- prediction
    // predict the position along avg velocity for (timeMessageArrived - timeMessageWasSent) amount
    sf::Vector2f displacement(pos0.x - pos1.x, pos0.y - pos1.y);
    float time = (float)(pos0.time > pos1.time ? (pos0.time - pos1.time) : 0) / 1000.f;

    sf::Vector2f velocity = time > 0 ? displacement / time : sf::Vector2f(0, 0);
    float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (speed > Player::MaxSpeed) {
        velocity = velocity / speed * Player::MaxSpeed;
    }

    float deltaTime = (float)(mSyncSnapshot.time > pos0.time ? (mSyncSnapshot.time - pos0.time) : 0) / 1000.f; // the time when snapshot arrived - the time it was sent from server
    sf::Vector2f prediction = sf::Vector2f(pos0.x, pos0.y) + velocity * deltaTime;
    //---


    float distSq = pow(prediction.x - mSyncSnapshot.x, 2) + pow(prediction.y - mSyncSnapshot.y, 2);

    float dt = (float)(gameTime > mSyncSnapshot.time ? gameTime - mSyncSnapshot.time : 0);
    float totalT = (float)(mSyncSnapshot.time > pos1.time ? mSyncSnapshot.time - pos1.time : 0);
    float alpha = totalT > 0 ? dt / totalT : 0;
    alpha = std::max(std::min(alpha, 1.f), 0.f);

    return sf::Vector2f(mSyncSnapshot.x, mSyncSnapshot.y) * (1 - alpha) + prediction * alpha;
}

sf::Vector2f Player::simulateMovement(const sf::Vector2f& movementForce, sf::Time dt, const sf::Vector2f& velocity, sf::Vector2f& newVelocity)
{
    float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    bool isForceApplied = (movementForce.x != 0 || movementForce.y != 0);
    const float epsilon = 1e-5f;

    if (isForceApplied) {
        sf::Vector2f netForce = movementForce;
        sf::Vector2f acceleration = netForce / 1.f; // assume mass = 1
        newVelocity = velocity + acceleration * dt.asSeconds();

        speed = sqrt(newVelocity.x * newVelocity.x + newVelocity.y * newVelocity.y);

        // clamp the velocity to max speed
        if (speed > Player::MaxSpeed) {
            newVelocity = newVelocity / speed * Player::MaxSpeed;
        }

        return newVelocity * dt.asSeconds(); // position increment
    }
    else {
        newVelocity = sf::Vector2f(0, 0); // make velocity zero
        return sf::Vector2f(0, 0); // no position increment

        //TODO: if you have enough time, try to bring this back and simulate friction/drag

        // If no movement force is applied, apply friction and check for very small velocities
        //if (speed > epsilon) {
        //    sf::Vector2f velocityNorm = velocity / speed;
        //    float frictionCoefficient = 10.0f; // Static friction coefficient for stopping
        //    sf::Vector2f frictionForce = -velocityNorm * frictionCoefficient * 1.f * 9.8f;

        //    // Apply friction as a deceleration
        //    sf::Vector2f netForce = frictionForce;
        //    sf::Vector2f acceleration = netForce / 1.f; // Assuming mass = 1 for simplicity
        //    this->accelerate(acceleration * dt.asSeconds());

        //    // Update velocity after friction is applied
        //    velocity = this->getVelocity();
        //    speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

        //    setPosition(getPosition() + velocity * dt.asSeconds());

        //    // Stop the object if the velocity is now very small
        //    if (speed < epsilon) {
        //        velocity = sf::Vector2f(0.f, 0.f);
        //        this->setVelocity(velocity);
        //    }
        //}
    }
}

sf::Color Player::getColor() const
{
	return mColor;
}

int Player::getScore() const
{
    return mScore;
}

void Player::setScore(int score)
{
    mScore = score;
}

void Player::updateCurrent(sf::Time dt, unsigned short playerId)
{
    mScoreIndicator.setString(std::to_string(mScore));
    mScoreIndicator.setPosition(getPosition().x, getPosition().y - 45);


    // if current player id is not playerId(main) and > 0, run prediction
    if (playerId >= 0 && mId != playerId)
    {
        sf::Vector2f interpolation = runInterpolation();

        setPosition(interpolation);
        //std::cout << interpolation.x << ", " << interpolation.y << std::endl;
        return;
    }

    if (mId == playerId && ( mServerCorrection.x > 0 || mServerCorrection.y > 0))
    {
        
    }
    

    //if (mCorrectionTime >=0 &&  mCorrectionTime < CorrectionPeriod) {
    //    mCorrectionTime += dt.asSeconds();

    //    // Interpolation alpha (normalized time)
    //    float alpha = mCorrectionTime / CorrectionPeriod;
    //    alpha = std::min(alpha, 1.0f); // Clamp to 1

    //    // Smoothly interpolate between the local position and the server position
    //    //setPosition(getPosition() * (1.0f - alpha) + mServerCorrection.position * alpha);

    //    sf::Vector2f velocityCorrection = mServerCorrection.position * 0.1f;
    //    setVelocity(getVelocity() + velocityCorrection * dt.asSeconds());

    //    // Optional: Add velocity adjustment for continuity
    //    //velocity += (correctionTarget - correctionStart) / correctionDuration * deltaTime;
    //}



    //sf::Vector2f positionInc, newVelocity;
    //positionInc = simulateMovement(mMovementForce, dt, getVelocity(), newVelocity);
    //setVelocity(newVelocity);
    //setPosition(getPosition() + positionInc);

    //// Reset movement force for the next frame
    //mMovementForce.x = 0;
    //mMovementForce.y = 0;
}

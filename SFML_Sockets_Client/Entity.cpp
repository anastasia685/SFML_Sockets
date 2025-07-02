#include "Entity.h"

void Entity::updateCurrent(sf::Time dt)
{
    // move func from transformable interface - internally calls setposition(prevpos + offset)
    move(mVelocity * dt.asSeconds()); // update
}

void Entity::setVelocity(sf::Vector2f velocity)
{
    mVelocity = velocity;
}
void Entity::setVelocity(float vx, float vy)
{
    mVelocity.x = vx;
    mVelocity.y = vy;
}
sf::Vector2f Entity::getVelocity() const
{
    return mVelocity;
}

void Entity::accelerate(sf::Vector2f vInc)
{
    mVelocity += vInc;
}

void Entity::accelerate(float vx, float vy)
{
    mVelocity.x += vx;
    mVelocity.y += vy;
}

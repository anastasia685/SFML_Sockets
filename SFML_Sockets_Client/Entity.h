#pragma once

#include "SceneNode.h"

class Entity : public SceneNode
{
private:
    virtual void updateCurrent(sf::Time dt);
private:
    sf::Vector2f mVelocity;

public:
    void setVelocity(sf::Vector2f velocity);
    void setVelocity(float vx, float vy);
    sf::Vector2f getVelocity() const;
    void accelerate(sf::Vector2f vInc);
    void accelerate(float vx, float vy);
};

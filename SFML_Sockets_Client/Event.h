#pragma once
#include <functional>
#include "SceneNode.h"
struct Event
{
    unsigned int id;
    unsigned int category;
    std::function<void(SceneNode&, sf::Time)> callback;
};
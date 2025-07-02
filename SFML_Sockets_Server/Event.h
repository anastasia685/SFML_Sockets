#pragma once
#include <functional>
#include <SFML/Graphics.hpp>

namespace Server
{
    struct Event
    {
        unsigned int id;
        sf::Vector2f move;
        float dt;
        //std::function<void(Player&)> callback;
    };
}

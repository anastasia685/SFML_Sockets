#pragma once
#include <queue>
#include "Event.h"

class EventQueue
{
public:
    void push(const Event& command);
    Event pop();
    bool isEmpty() const;
private:
    std::queue<Event> mQueue;
};

#include "EventQueue.h"

void EventQueue::push(const Event& command)
{
	mQueue.push(command);
}

Event EventQueue::pop()
{
	Event command = mQueue.front();
	mQueue.pop();
	return command;
}

bool EventQueue::isEmpty() const
{
	return mQueue.empty();
}

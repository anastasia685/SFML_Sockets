#include "EventQueue.h"

void Server::EventQueue::push(const Event& move)
{
	mQueue.push(move);
}

Server::Event Server::EventQueue::pop()
{
	Event command = mQueue.front();
	mQueue.pop();
	return command;
}

bool Server::EventQueue::isEmpty() const
{
	return mQueue.empty();
}

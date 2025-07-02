#include "SceneNode.h"
#include "Event.h"
#include <cassert>

SceneNode::SceneNode() : mChildren() , mParent(nullptr){}

void SceneNode::addChild(SceneNode::Ptr child)
{
	child->mParent = this;
	mChildren.push_back(std::move(child)); // move the ownership to this class
}
SceneNode::Ptr SceneNode::removeChild(const SceneNode& node)
{
	auto found = std::find_if(mChildren.begin(), mChildren.end(), [&](Ptr& p) -> bool { return p.get() == &node; });
	assert(found != mChildren.end());

	Ptr result = std::move(*found); // relinquish ownership
	result->mParent = nullptr; // no longer managed by this parent
	mChildren.erase(found);
	return result;
}

const std::vector<SceneNode::Ptr>& SceneNode::getChildren() const
{
	return mChildren;
}

void SceneNode::update(sf::Time dt, unsigned short playerId)
{
	updateCurrent(dt, playerId);

	for (const Ptr& child : mChildren)
	{
		child->update(dt, playerId);
	}

	auto it = std::remove_if(
		mChildren.begin(),
		mChildren.end(),
		[](const std::unique_ptr<SceneNode>& child) {
			return child->shouldRemove();
		}
	);
	mChildren.erase(it, mChildren.end());

}

sf::Transform SceneNode::getWorldTransform() const
{
	// initial identity matrix
	sf::Transform transform = sf::Transform::Identity;

	// iterate up the node tree hierarchy & accumulate relative transforms to get world
	for (const SceneNode* node = this; node != nullptr; node = node->mParent)
		transform *= node->getTransform();

	return transform;
}

sf::Vector2f SceneNode::getWorldPosition() const
{
	// worldtransform returns 3x3 transformation matrix, we multiply it by current local position(which will always be (0,0) to get world position
	return getWorldTransform() * sf::Vector2f();
}

unsigned int SceneNode::getCategory() const
{
	return Type::None;
}
unsigned int SceneNode::getId() const
{
	return 0;
}
void SceneNode::onEvent(const Event& event, sf::Time dt)
{
	// execute event callback if category matches
	if (event.category & getCategory() && (event.id == getId()))
		event.callback(*this, dt);

	// propagate event to children
	for (Ptr& child : mChildren)
	{
		child->onEvent(event, dt);
	}
}

bool SceneNode::shouldRemove() const
{
	return mFlaggedForRemoval;
}

void SceneNode::flagForRemoval()
{
	mFlaggedForRemoval = true;
}

void SceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	states.transform *= getTransform(); // apply current relative transformation

	// draw parent
	drawCurrent(target, states);

	// draw children (recursively)
	for (const Ptr& child : mChildren)
	{
		child->draw(target, states);
	}
}

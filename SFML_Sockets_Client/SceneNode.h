#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

struct Event;

// transformable for setting transformations (translation, rotation, scaling)
// drawable to override draw function and pass this directly to window.draw()
// non-copyable to disable copy constructor, assignment op, etc
class SceneNode : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
{
public:
    typedef std::unique_ptr<SceneNode> Ptr; //unique ptr to each child to simplify memory management

    enum Type
    {
        None = 0,
        Player = 1 << 0,
        Bullet = 2 << 0,
    };

public:
    SceneNode();

    void addChild(Ptr child);
    Ptr removeChild(const SceneNode& node);
    const std::vector<Ptr>& getChildren() const;
    void update(sf::Time dt, unsigned short playerId);
    sf::Transform getWorldTransform() const;
    sf::Vector2f getWorldPosition() const;

    virtual unsigned int getCategory() const;
    virtual unsigned int getId() const;
    void onEvent(const Event& event, sf::Time dt);

    bool shouldRemove() const;
    void flagForRemoval();

// for implementing drawable interface
private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    // for drawing only this(parent) entity, not its children
    // must be overriden in derived classes
    virtual void drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const {};

    // for updating only this(parent) entity, not its children
    // must be overriden in entity general class
    virtual void updateCurrent(sf::Time dt, unsigned short playerId) {};

private:
    std::vector<Ptr> mChildren;
    SceneNode* mParent;

    bool mFlaggedForRemoval;
};

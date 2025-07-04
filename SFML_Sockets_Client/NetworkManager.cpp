#include "NetworkManager.h"
#include <iostream>


NetworkManager::NetworkManager()
{
    mSocket.bind(sf::Socket::AnyPort);
    mSocket.setBlocking(false);

    // add the udp socket to the selector
    mSelector.add(mSocket);
}

void NetworkManager::update(Player** player, SceneNode& rootPlayerNode, SceneNode& rootBulletNode, EventQueue& events, sf::Time dt)
{
    mTimeSend += dt;
    mTimeRead += dt;

    receive(player, rootPlayerNode, rootBulletNode);

    // apply messages
    if (mTimeRead.asSeconds() > 0.08)
    {
        const std::vector<SceneNode::Ptr>& players = rootPlayerNode.getChildren();
        PlayerMessage msg;

        while (hasMessages())
        {
            msg = processMessage(); // pop the queue

            switch ((PacketType)msg.type)
            {
            case PacketType::PlayerState:
            {
                // if new entity id, add to scene graph
                auto found = std::find_if(players.begin(), players.end(), [&msg](const SceneNode::Ptr& p) -> bool { return p.get()->getId() == msg.id; });
                if (found == players.end())
                {
                    // insert new scene node
                    std::unique_ptr<Player> player(new Player(msg.id, sf::Color(sf::Color::Magenta)));
                    player->setPosition(msg.x, msg.y);
                    player->setScore(msg.score);
                    rootPlayerNode.addChild(std::move(player));
                }

                // local player sync
                if (msg.id == (*player)->getId())
                {
                    if ((*player)->getMovesCache().empty())
                    {
                        // just set
                        Event e;
                        e.id = msg.id;
                        e.category = SceneNode::Type::Player;
                        e.callback = [msg](SceneNode& s, sf::Time dt)
                            {
                                Player& player = static_cast<Player&>(s);
                                player.setPosition(msg.x, msg.y);
                                player.setScore(msg.score);
                            };
                        events.push(e);
                        continue;
                    }
                    if ((*player)->getMovesCache().empty() || (*player)->getMovesCache().front().timestamp > msg.endMove)
                    {
                        //skip if sync message is too old
                        continue;
                    }

                    Event e;
                    e.id = msg.id;
                    e.category = SceneNode::Type::Player;
                    e.callback = [msg](SceneNode& s, sf::Time dt)
                    {
                        Player& player = static_cast<Player&>(s);

                        // set state to snapshot data

                        // this is for latest applied move + (timestamp-lastMoveTimestamp) simulation with 0
                        sf::Vector2f position = sf::Vector2f(msg.x, msg.y);
                        sf::Vector2f velocity;


                        sf::Vector2f pendingMovesIncrement = sf::Vector2f(0, 0), _v;

                        float epsilon = 0.005f; 
                        uint32_t gapTime;
                        for (auto i = player.getMovesCache().begin(); i != player.getMovesCache().end(); i++)
                        {
                            auto& move = *i;

                            // skipped (dropped) moves (that will be resent)
                            if (move.timestamp >= msg.startMove && move.timestamp <= msg.endMove)
                            {
                                bool acked;
                                if (msg.startMove == msg.endMove) // only 1 ack, value will be set at max_moves/2 index
                                {
                                    acked = move.timestamp == msg.startMove && msg.movesBitmap.test(MAX_MOVES / 2);
                                }
                                else
                                {
                                    size_t index = ((move.timestamp - msg.startMove) * (MAX_MOVES - 1)) / (msg.endMove - msg.startMove);
                                    acked = msg.movesBitmap.test(index);
                                }


                                if (!acked)
                                {
                                    // unAcked move within snapshot [start,end] range, estimate position change contribution of this move
                                    pendingMovesIncrement += player.simulateMovement(move.force, sf::seconds(move.dt), move.velocity, _v);

                                    auto next = std::next(i);
                                    if (next != player.getMovesCache().end())
                                    {
                                        gapTime = next->timestamp - (move.timestamp + sf::seconds(move.dt).asMilliseconds());
                                        if (gapTime > epsilon)
                                        {
                                            position += player.simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(gapTime), _v, _v);
                                        }
                                    }
                                }
                                else
                                {
                                    // could also do this right when receiving instead of waiting until it's time to apply
                                    move.acked = true;
                                }
                            }
                            // past unacked
                            else if (move.timestamp < msg.startMove && !move.acked)
                            {
                                pendingMovesIncrement += player.simulateMovement(move.force, sf::seconds(move.dt), move.velocity, _v);
                                auto next = std::next(i);
                                if (next != player.getMovesCache().end())
                                {
                                    gapTime = next->timestamp - (move.timestamp + sf::seconds(move.dt).asMilliseconds());
                                    if (gapTime > epsilon)
                                    {
                                        position += player.simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(gapTime), _v, _v);
                                    }
                                }
                            }
                            // future moves
                            else if (move.timestamp > msg.endMove)
                            {
                                position += player.simulateMovement(move.force, sf::seconds(move.dt), move.velocity, velocity);

                                auto next = std::next(i);
                                if (next != player.getMovesCache().end())
                                {
                                    gapTime = next->timestamp - (move.timestamp + sf::seconds(move.dt).asMilliseconds());
                                    if (gapTime > epsilon)
                                    {
                                        position += player.simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(gapTime), velocity, velocity);
                                    }
                                }
                            }
                        }

                        player.setVelocity(velocity);
                        player.setPosition(position + pendingMovesIncrement);
                        player.setScore(msg.score);


                        for (auto& shot : player.getShotsCache())
                        {
                            // skipped (dropped) moves (that will be resent)
                            if (shot.timestamp >= msg.startShot && shot.timestamp <= msg.endShot)
                            {
                                bool acked;
                                if (msg.startShot == msg.endShot) // only 1 ack, value will be set at max_moves/2 index
                                {
                                    acked = shot.timestamp == msg.startShot && msg.shotsBitmap.test(MAX_SHOTS / 2);
                                }
                                else
                                {
                                    size_t index = ((shot.timestamp - msg.startShot) * (MAX_SHOTS - 1)) / (msg.endShot - msg.startShot);
                                    acked = msg.shotsBitmap.test(index);
                                }

                                shot.acked = acked;
                            }
                        }

                    };
                    events.push(e);
                }
                // remote player update
                else if (msg.id != (*player)->getId())
                {
                    Event e;
                    e.id = msg.id;
                    e.category = SceneNode::Type::Player;
                    e.callback = [msg](SceneNode& s, sf::Time)
                        {
                            Player& player = static_cast<Player&>(s);
                            // cache current position to interpolate between that and new latest snapshot
                            auto now = std::chrono::high_resolution_clock::now();
                            auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                            uint32_t truncated_timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);
                            player.setSyncSnapshot(player.getPosition(), truncated_timestamp);
                            // update snapshots queue
                            player.updatePositions(msg.x, msg.y, msg.timestamp);
                            player.setScore(msg.score);
                        };
                    events.push(e);
                }
                break;
            }
            case PacketType::BulletsState:
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                uint32_t truncated_timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);

                if (truncated_timestamp - msg.timestamp >= sf::seconds(Bullet::Range / Bullet::Speed).asMilliseconds())
                {
                    break;
                }

                sf::Vector2f direction = sf::Vector2f(msg.vx, msg.vy);
                sf::Vector2f position = sf::Vector2f(msg.x, msg.y) + sf::milliseconds(truncated_timestamp - msg.timestamp).asSeconds() * direction * Bullet::Speed;
                std::unique_ptr<Bullet> bullet(new Bullet(msg.playerId, msg.timestamp, {msg.x, msg.y}, direction, msg.playerId != (*player)->getId()));
                bullet->setPosition(position);
                rootBulletNode.addChild(std::move(bullet));

                break;
            }
            }
        }


        mTimeRead = sf::Time::Zero;
    }
}
void NetworkManager::send(Player* player)
{
    //send
    if (mTimeSend.asSeconds() >= 0.05)
    {
        sf::Packet packet;
        sf::Socket::Status status;
        if (player != nullptr)
        {
            //(*player)->mReset = true;

            sf::Vector2f position = player->getPosition();
            packet << sf::Uint32(PacketType::PlayerUpdate) << sf::Uint32(player->getId()) << position.x << position.y;

            // send unacked moves
            uint32_t moveCount = 0;
            for (const auto& move : player->getMovesCache()) {
                if (!move.acked) 
                {
                    moveCount++;
                }
            }
            packet << sf::Uint32(moveCount);
            if (moveCount > 0)
            {
                for (const auto& m : player->getMovesCache())
                {
                    if (!m.acked)
                    {
                        packet << m.force.x << m.force.y << m.dt << sf::Uint32(m.timestamp);
                    }
                }
            }

            // send unacked shots
            uint32_t shotCount = 0;
            for (const auto& shot : player->getShotsCache()) {
                if (!shot.acked)
                {
                    shotCount++;
                }
            }
            packet << sf::Uint32(shotCount);
            if (shotCount > 0)
            {
                for (const auto& s : player->getShotsCache())
                {
                    if (!s.acked)
                    {
                        packet << sf::Uint32(s.timestamp) << s.spawnPosition.x << s.spawnPosition.y << s.direction.x << s.direction.y;
                    }
                }
            }
            
            if (moveCount > 0 || shotCount > 0)
            {
                status = mSocket.send(packet, SERVER_IP, SERVER_PORT);
            }
        }
        else
        {
            // send hello message
            packet << sf::Uint32(PacketType::Hello);
            status = mSocket.send(packet, SERVER_IP, SERVER_PORT);
        }

        // reset the timer
        mTimeSend = sf::Time::Zero;
    }
}
void NetworkManager::receive(Player** player, SceneNode& rootPlayerNode, SceneNode& rootBulletNode)
{
    const std::vector<SceneNode::Ptr>& players = rootPlayerNode.getChildren();
    const std::vector<SceneNode::Ptr>& bullets = rootBulletNode.getChildren();

    sf::IpAddress sender;
    unsigned short port;
    sf::Socket::Status status;
    sf::Packet packet;

    PlayerMessage msg;
    if (mSelector.wait(sf::microseconds(1)))
    {
        if (mSelector.isReady(mSocket))
        {
            for (int i = 0; i < 5; i++)
            {
                status = mSocket.receive(packet, sender, port);
                if (status == sf::Socket::Done)
                {
                    // add to event queue
                    packet >> msg.type;

                    switch ((PacketType)msg.type)
                    {
                    case PacketType::Welcome:
                    {
                        // main player already set
                        if (*player != nullptr) break;

                        // send server-assigned player id
                        unsigned short playerId;
                        packet >> playerId;

                        const sf::Color colorArray[5] = { sf::Color::Cyan, sf::Color::Blue, sf::Color::Green, sf::Color::Yellow };
                        srand(time(NULL));

                        // generate world entity
                        std::unique_ptr<Player> p(new Player(playerId, sf::Color::Cyan /*colorArray[rand() % 5]*/));
                        p->setPosition(400, 300);
                        rootPlayerNode.addChild(std::move(p));

                        // assign root player pointer to newly created world entity
                        auto found = std::find_if(players.begin(), players.end(), [&playerId](const SceneNode::Ptr& p) -> bool { return p.get()->getId() == playerId; });
                        if (found != players.end())
                            *player = static_cast<Player*>(found->get());

                        break;
                    }
                    case PacketType::Remove:
                    {
                        unsigned short playerId;
                        packet >> playerId;

                        auto found = std::find_if(players.begin(), players.end(), [&playerId](const SceneNode::Ptr& p) -> bool { return p.get()->getId() == playerId; });
                        if (found != players.end())
                        {
                            auto sceneNode = found->get();
                            rootPlayerNode.removeChild(*sceneNode);
                        }

                        break;
                    }
                    case PacketType::PlayerState:
                    {
                        // disable updates until join acknowledged
                        if (*player == nullptr) break;

                        sf::Uint32 score;
                        packet >> msg.id >> msg.x >> msg.y >> score >> msg.timestamp;
                        msg.score = (int)score * -1;

                        packet >> msg.startMove >> msg.endMove >> msg.movesBitmap >> msg.startShot >> msg.endShot >> msg.shotsBitmap;

                        // buffer incoming position update
                        bufferMessage(msg);

                        break;
                    }
                    case PacketType::BulletsState:
                    {
                        if (*player == nullptr) break;

                        uint32_t bulletCount;
                        packet >> bulletCount;

                        for (int i = 0; i < bulletCount; i++)
                        {
                            packet >> msg.playerId >> msg.timestamp >> msg.x >> msg.y >> msg.vx >> msg.vy;

                            // if already in scene, don't do anything
                            auto found = std::find_if(bullets.begin(), bullets.end(), [&msg](const SceneNode::Ptr& p) -> bool 
                                {
                                    Bullet* b = static_cast<Bullet*>(p.get());
                                    return b->getPlayerId() == msg.playerId && b->getTimestamp() == msg.timestamp;
                                });

                            if (found == bullets.end())
                            {
                                bufferMessage(msg);
                            }
                        }

                        break;
                    }
                    }

                    packet.clear();
                }
                else if (status == sf::Socket::NotReady)
                {
                    break;
                }
            }
        }
    }
}

void NetworkManager::bufferMessage(const PlayerMessage& msg)
{
    auto it = std::lower_bound(
        mMessageQueue.begin(),
        mMessageQueue.end(),
        msg,
        [](const PlayerMessage& a, const PlayerMessage& b)
        {
            return a.timestamp < b.timestamp;
        });

    // replace if present
    if (it != mMessageQueue.end() && it->timestamp == msg.timestamp && it->id == msg.id)
    {
        *it = msg;
    }
    // insert
    else
    {
        // Insert the new move at the correct position
        mMessageQueue.insert(it, msg);
    }


    //mQueue.push(msg);
}

PlayerMessage NetworkManager::processMessage()
{
    PlayerMessage msg = mMessageQueue.front();
    mMessageQueue.pop_front();
    return msg;
}

bool NetworkManager::hasMessages() const
{
    return !mMessageQueue.empty();
}

void NetworkManager::setServerIp(std::string ip)
{
    SERVER_IP = ip;
}

void NetworkManager::setServerPort(int port)
{
    SERVER_PORT = port;
}

sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_MOVES>& bitmap)
{
    // serialize byte-by-byte
    for (size_t i = 0; i < MAX_MOVES; i += 8) 
    {
        uint8_t byte = 0;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_MOVES; bit++)
        {
            if (bitmap.test(i + bit)) 
            {
                byte |= (1 << bit);
            }
        }
        packet << byte;
    }
    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_MOVES>& bitmap) 
{
    // extract byte-by-byte
    for (size_t i = 0; i < MAX_MOVES; i += 8) 
    {
        uint8_t byte = 0;
        packet >> byte;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_MOVES; bit++) 
        {
            if (byte & (1 << bit)) 
            {
                bitmap.set(i + bit);
            }
            else 
            {
                bitmap.reset(i + bit);
            }
        }
    }
    return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const std::bitset<MAX_SHOTS>& bitmap)
{
    // serialize byte-by-byte
    for (size_t i = 0; i < MAX_SHOTS; i += 8)
    {
        uint8_t byte = 0;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_SHOTS; bit++)
        {
            if (bitmap.test(i + bit))
            {
                byte |= (1 << bit);
            }
        }
        packet << byte;
    }
    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, std::bitset<MAX_SHOTS>& bitmap)
{
    // extract byte-by-byte
    for (size_t i = 0; i < MAX_SHOTS; i += 8)
    {
        uint8_t byte = 0;
        packet >> byte;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_SHOTS; bit++)
        {
            if (byte & (1 << bit))
            {
                bitmap.set(i + bit);
            }
            else
            {
                bitmap.reset(i + bit);
            }
        }
    }
    return packet;
}
#include "NetworkManager.h"
#include "Game.h"
#include <iostream>

using namespace Server;



NetworkManager::NetworkManager(Game* game) : mGame(game)
{
    mSocket.bind(SERVER_PORT);
    mSocket.setBlocking(false);

    // Add the listener to the selector
    mSelector.add(mSocket);
}

void NetworkManager::update(sf::Time dt)
{
    mTime += dt;

    receive();

    // add buffered messages to event queue to be applied just before sending
    if (mTime.asSeconds() >= 0.08)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        uint32_t timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);

        while (hasMessages())
        {
            PlayerMessage update = processMessage();

            Player* player = mGame->getPlayer(update.playerID);

            if (player == nullptr) continue;

            switch (update.type)
            {
            case UpdateType::Move:
            {
                // already applied
                if (!player->getMovesCache().empty())
                {
                    auto found = std::find_if(player->getMovesCache().begin(), player->getMovesCache().end(), [&update](const Player::Move& m) -> bool { return update.timestamp == m.timestamp; });
                    if (found != player->getMovesCache().end())
                    {
                        continue;
                    }
                }

                Player::Move newMove = Player::Move({ update.force, update.dt, update.timestamp });

                auto found = std::lower_bound(player->getMovesCache().begin(), player->getMovesCache().end(), newMove,
                    [](const Player::Move& a, const Player::Move& b) {
                        return a.timestamp < b.timestamp;
                    });

                if (found != player->getMovesCache().end() && found != player->getMovesCache().begin())
                {
                    auto previous = std::prev(found);
                    // simulate found to get new velocity
                    sf::Vector2f positionInc, newVelocity;
                    positionInc = player->simulateMovement(previous->force, sf::seconds(previous->dt), previous->velocity, newVelocity);


                    // simulate the gap between the end of prev and new move
                    float epsilon = 5.f;
                    uint32_t dt = newMove.timestamp - (previous->timestamp + sf::seconds(previous->dt).asMilliseconds());
                    if (dt > epsilon)
                    {
                        positionInc += player->simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(dt), newVelocity, newVelocity);
                    }

                    newMove.velocity = newVelocity;
                    newMove.position = previous->position + positionInc;
                    newMove.score = previous->score;
                }
                else
                {
                    newMove.position = player->getPosition();
                    newMove.velocity = player->getVelocity();
                    newMove.score = player->getScore();
                }

                // starting point
                sf::Vector2f position = newMove.position, velocity = newMove.velocity;
                int score = newMove.score;

                // apply new move
                position += player->simulateMovement(newMove.force, sf::seconds(newMove.dt), newMove.velocity, velocity);
                // re-check bullet collision
                for (auto& bullet : mGame->getBullets())
                {
                    uint32_t currTime = newMove.timestamp + sf::seconds(newMove.dt).asMilliseconds();
                    uint32_t bulletSimTime = currTime - bullet->getTimestamp();
                    if (
                        player->getId() == bullet->getPlayerId() ||
                        bullet->getTimestamp() > currTime ||
                        bulletSimTime > sf::seconds(Bullet::Range / Bullet::Speed).asMilliseconds() ||
                        bullet->hasHitPlayer(player->getId())
                        )
                    {
                        continue;
                    }
                        

                    // get bullet's position at this time
                    auto bulletPosition = bullet->simulateMovement(sf::milliseconds(currTime - bullet->getTimestamp()));
                    // check for collision
                    if (Bullet::checkCollision(bulletPosition, position))
                    {
                        score--;
                        bullet->markPlayerAsHit(player->getId());
                    }
                }


                // simulate the gap after the new move and found
                float epsilon = 5; // in millis
                uint32_t gap;
                if (found != player->getMovesCache().end())
                {
                    gap = found->timestamp - (newMove.timestamp + sf::seconds(newMove.dt).asMilliseconds());
                    if (gap > epsilon)
                    {
                        // simulate everything up to found->timestamp
                        position += player->simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(gap), velocity, velocity);

                        // re-check bullet collision
                        for (auto& bullet : mGame->getBullets())
                        {
                            uint32_t bulletSimTime = found->timestamp - bullet->getTimestamp();
                            if (
                                player->getId() == bullet->getPlayerId() ||
                                bullet->getTimestamp() > found->timestamp ||
                                bulletSimTime > sf::seconds(Bullet::Range/Bullet::Speed).asMilliseconds() ||
                                bullet->hasHitPlayer(player->getId())
                                )
                            {
                                continue;
                            }

                            // get bullet's position at this time
                            auto bulletPosition = bullet->simulateMovement(sf::milliseconds(found->timestamp - bullet->getTimestamp()));
                            // check for collision
                            if (Bullet::checkCollision(bulletPosition, position))
                            {
                                score--;
                                bullet->markPlayerAsHit(player->getId());
                            }
                        }
                    }
                }

                // reapply cached moves
                for (auto i = found; i != player->getMovesCache().end(); i++)
                {
                    Player::Move& move = *i;
                    // update each future move's starting data
                    move.position = position;
                    move.velocity = velocity;
                    move.score = score;

                    position += player->simulateMovement(move.force, sf::seconds(move.dt), move.velocity, velocity);
                    // re-check bullet collision
                    for (auto& bullet : mGame->getBullets())
                    {
                        uint32_t currTime = move.timestamp + sf::seconds(move.dt).asMilliseconds();
                        uint32_t bulletSimTime = currTime - bullet->getTimestamp();

                        if (
                            player->getId() == bullet->getPlayerId() ||
                            bullet->getTimestamp() > currTime ||
                            bulletSimTime > sf::seconds(Bullet::Range/Bullet::Speed).asMilliseconds() ||
                            bullet->hasHitPlayer(player->getId())
                            )
                        {
                            continue;
                        }

                        // get bullet's position at this time
                        auto bulletPosition = bullet->simulateMovement(sf::milliseconds(currTime - bullet->getTimestamp()));
                        // check for collision
                        if (Bullet::checkCollision(bulletPosition, position))
                        {
                            score--;
                            bullet->markPlayerAsHit(player->getId());
                        }
                    }

                    // simulate the gap to the next move
                    auto next = std::next(i);
                    if (next != player->getMovesCache().end())
                    {
                        gap = next->timestamp - (i->timestamp + sf::seconds(move.dt).asMilliseconds());
                        if (gap > epsilon)
                        {
                            // simulate everything up to next->timestamp
                            position += player->simulateMovement(sf::Vector2f(0, 0), sf::milliseconds(gap), velocity, velocity);
                            // re-check bullet collision
                            for (auto& bullet : mGame->getBullets())
                            {
                                uint32_t bulletSimTime = next->timestamp - bullet->getTimestamp();

                                if (
                                    player->getId() == bullet->getPlayerId() ||
                                    bullet->getTimestamp() > next->timestamp ||
                                    bulletSimTime > sf::seconds(Bullet::Range/Bullet::Speed).asMilliseconds() || 
                                    bullet->hasHitPlayer(player->getId())
                                    )
                                {

                                    continue;
                                }

                                // get bullet's position at this time
                                auto bulletPosition = bullet->simulateMovement(sf::milliseconds(next->timestamp - bullet->getTimestamp()));
                                // check for collision
                                if (Bullet::checkCollision(bulletPosition, position))
                                {
                                    score--;
                                    bullet->markPlayerAsHit(player->getId());
                                }
                            }
                        }
                    }
                }

                // cache new move
                player->updateMovesCache(newMove);


                // update player data
                player->setVelocity(velocity);
                player->setPosition(position);
                player->setScore(score);

                break;
            }
            case UpdateType::Shot:
            {
                // handle shot requests

                // find first in cache with <= timestamp
                // if delta < shootrate - skip
                // else - add to mbullets

                if (!player->getShotsCache().empty())
                {
                    auto found = std::find_if(player->getShotsCache().begin(), player->getShotsCache().end(), [&update](const Player::Shot& s) -> bool { return update.timestamp <= s.timestamp; });
                    if (found != player->getShotsCache().end() && update.timestamp - found->timestamp < Bullet::ShotRate)
                    {
                        break; // already applied if equal, or not allowed to shoot if small delta
                    }
                }
                //---- handling cache update
                player->updateShotsCache(update.timestamp, update.position, update.force);


                //---- handling global state update
                // check if mbullets already has instance
                auto found = std::find_if(mGame->getBullets().begin(), mGame->getBullets().end(), [&update](const BulletPtr& ptr) -> bool { return update.playerID == ptr->getPlayerId() && update.timestamp <= ptr->getTimestamp(); });
                if (found != mGame->getBullets().end())
                {
                    break;
                }

                // check every hostile player for collisions up to this point
                auto now = std::chrono::high_resolution_clock::now();
                auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                uint32_t timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);

                uint32_t gap = timestamp - update.timestamp;
                float timeStep = 1.0f / 60.0f;

                sf::Vector2f bulletPosition = update.position;
                sf::Vector2f bulletDirection = update.force;

                std::unique_ptr<Bullet> bullet(new Bullet(update.playerID, update.timestamp, update.position, update.force));

                // Simulate bullet over time steps
                while (gap > 0.0f) {
                    // Advance bullet position by timeStep
                    bulletPosition += bulletDirection * (Bullet::Speed * timeStep);

                    // Get each player's position at the current simulation time
                    //float simulatedTime = gap - remainingTime; // Time elapsed since bullet spawn
                    
                    uint32_t currTime = update.timestamp + (timestamp - gap);

                    for (auto& otherPlayer : mGame->getPlayers()) 
                    {
                        // don't hit yourself :d
                        if (otherPlayer->getId() == update.playerID) continue;

                        if (!otherPlayer->getMovesCache().empty())
                        {
                            auto found = std::find_if(otherPlayer->getMovesCache().rbegin(), otherPlayer->getMovesCache().rend(),
                                [&update, currTime](const Player::Move& m) -> bool {
                                    return m.timestamp <= currTime; // Find the last move before or at the bullet's timestamp
                                });

                            if (found == otherPlayer->getMovesCache().rend())
                            {
                                continue;
                            }

                            // Use the found move
                            const Player::Move& lastMove = *found;
                            auto playerPosition = lastMove.position;
                            auto velocity = lastMove.velocity;
                            // Process the move (rewind player position)
                            float epsilon = 0.005; // in secs
                            float extraTimespan = sf::milliseconds(currTime - lastMove.timestamp).asSeconds();
                            if (extraTimespan > epsilon)
                            {
                                //float fraction = sf::milliseconds(currTime - lastMove.timestamp).asSeconds();
                                playerPosition += otherPlayer->simulateMovement(lastMove.force, sf::milliseconds(std::min(lastMove.dt, extraTimespan)), velocity, velocity);

                                // still some left after simulating the whole move
                                if (extraTimespan - lastMove.dt > epsilon)
                                {
                                    // simulate the rest with 0
                                    playerPosition += otherPlayer->simulateMovement(sf::Vector2f(0, 0), sf::seconds(extraTimespan - lastMove.dt), velocity, velocity);
                                }
                            }

                            if (Bullet::checkCollision(bulletPosition, playerPosition))
                            {
                                if (!bullet->hasHitPlayer(player->getId()))
                                {

                                    // decrement players score;
                                    otherPlayer->setScore(otherPlayer->getScore() - 1);

                                    // update moves initial data
                                    for (auto it = found.base(); it != otherPlayer->getMovesCache().end(); ++it)
                                    {
                                        it->score--;
                                    }
                                    bullet->markPlayerAsHit(player->getId());
                                }
                            }
                        }
                    }

                    // Decrease remaining time
                    gap -= timeStep * 1000.f; // to millis
                }

                mGame->addBullet(std::move(bullet), timestamp);
                break;
            }
            }
        }
    }

    for (auto& player : mGame->getPlayers())
    {
        for (auto& bullet : mGame->getBullets())
        {
            if (bullet->getPlayerId() == player->getId() || bullet->hasHitPlayer(player->getId()))
            {
                continue;
            }
            if (Bullet::checkCollision(bullet->getPosition(), player->getPosition()))
            {
                player->setScore(player->getScore() - 1);

                bullet->markPlayerAsHit(player->getId());
            }
        }
    }
}

void Server::NetworkManager::send()
{
    // send game state snapshots
    sf::Packet packet;
    sf::Socket::Status status;
    if (mTime.asSeconds() >= 0.08)
    {
        for (auto client : mClients)
        {
            packet.clear();
            Player* player = mGame->getPlayer(client.second);

            if (player == nullptr) continue;

            // for now 
            auto now = std::chrono::high_resolution_clock::now();
            auto absolute_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            uint32_t timestamp = static_cast<uint32_t>(absolute_timestamp % 4294967295);
            uint32_t lastUpdateTimestamp = timestamp;
            //get bitmap of all cached moves
            std::bitset<MAX_MOVES> moveBitmap;
            uint32_t moveStartTimestamp = 0, moveEndTimestamp = 0, range = 0;
            if (player->getMovesCache().size() > 0)
            {
                moveStartTimestamp = player->getMovesCache().front().timestamp;
                moveEndTimestamp = player->getMovesCache().back().timestamp;
                range = moveEndTimestamp - moveStartTimestamp;

                // if only 1 move - range will be 0, so handle this as a special case
                if (moveStartTimestamp == moveEndTimestamp)
                {
                    moveBitmap.set(MAX_MOVES / 2);
                }
                else if (range > 0)
                {
                    for (auto& move : player->getMovesCache()) 
                    {
                        size_t index = ((move.timestamp - moveStartTimestamp) * (MAX_MOVES - 1)) / range;
                        moveBitmap.set(index);
                    }
                }

                lastUpdateTimestamp = (sf::milliseconds(moveEndTimestamp) + sf::seconds(player->getMovesCache().back().dt)).asMilliseconds();
            }

            std::bitset<MAX_SHOTS> shotBitmap;
            uint32_t shotStartTimestamp = 0, shotEndTimestamp = 0;
            if (player->getShotsCache().size() > 0)
            {
                shotStartTimestamp = player->getShotsCache().front().timestamp;
                shotEndTimestamp = player->getShotsCache().back().timestamp;
                range = shotEndTimestamp - shotStartTimestamp;

                if (shotStartTimestamp == shotEndTimestamp)
                {
                    shotBitmap.set(MAX_SHOTS / 2);
                }
                else if (range > 0)
                {
                    for (auto& shot : player->getShotsCache()) {
                        size_t index = ((shot.timestamp - shotStartTimestamp) * (MAX_SHOTS - 1)) / range;
                        shotBitmap.set(index);
                    }
                }
            }

            packet << sf::Uint32(PacketType::PlayerState) << sf::Uint32(player->getId()) << player->getPosition().x << player->getPosition().y << sf::Uint32(abs(player->getScore())) << sf::Uint32(lastUpdateTimestamp);
            //packet << sf::Uint32(PacketType::Update) << player->getId() << 0 << 0;

            // send each player curr player's position
            for (auto client1 : mClients)
            {
                // send ack data to local players only
                if (client1.second == player->getId())
                {
                    packet << moveStartTimestamp << moveEndTimestamp << moveBitmap << shotStartTimestamp << shotEndTimestamp << shotBitmap;
                }
                status = mSocket.send(packet, client1.first.first, client1.first.second);
                if (status != sf::Socket::Done) {
                    std::cout << "Send error. Status " << std::to_string((int)status) << std::endl;
                    continue;
                }
            }

            packet.clear();


            // send all the game bullets data
            if (mGame->getBullets().size() > 0)
            {
                packet << sf::Uint32(PacketType::BulletsState) << sf::Uint32(mGame->getBullets().size());
                for (const auto& bullet : mGame->getBullets())
                {
                    packet << sf::Uint32(bullet->getPlayerId()) << sf::Uint32(bullet->getTimestamp()) << bullet->getSpawnPosition().x << bullet->getSpawnPosition().y << bullet->getDirection().x << bullet->getDirection().y;
                }

                status = mSocket.send(packet, client.first.first, client.first.second);
                if (status != sf::Socket::Done) {
                    std::cout << "Send error. Status " << std::to_string((int)status) << std::endl;
                    continue;
                }
            }

        }
        mTime = sf::Time::Zero;
    }
}

void Server::NetworkManager::receive()
{
    if (mSelector.wait(sf::microseconds(1)))
        {
            // Test the listener
            if (mSelector.isReady(mSocket))
            {
                sf::Packet packet;
                sf::Socket::Status status;
                sf::IpAddress sender;
                unsigned short port;

                Message msg;

                for (int i = 0; i < 10; i++)
                {
                    status = mSocket.receive(packet, sender, port);
                    if (status == sf::Socket::Disconnected)
                    {
                        unsigned short playerId = mClients[{sender, port}];
                        mClients.erase({ sender, port });
                        mGame->removePlayer(playerId);

                        //for (auto client : mClients)
                        //{
                        //    // send removePlayer message to all other clients
                        //    packet.clear();
                        //    packet << sf::Uint32(PacketType::Remove) << playerId;
                        //    status = mSocket.send(packet, client.first.first, client.first.second);
                        //    if (status != sf::Socket::Done) {
                        //        std::cout << "RemovePlayer send error. Status " << std::to_string((int)status) << std::endl;
                        //    }
                        //}
                        continue;
                    }
                    if (status == sf::Socket::NotReady)
                    {
                        break;
                    }
                    if (status != sf::Socket::Done)
                    {
                        std::cout << "Receive error. Status " << std::to_string((int)status) << std::endl;
                        continue;
                    }
                    // TODO: validate per packet type
                    /*if (packet.getDataSize() != sizeof(Message))
                    {
                        std::cout << "Received strange-sized message: " << packet.getDataSize() << ". Expected " << sizeof(Message) << std::endl;
                        return;
                    }*/

                    sf::Uint32 type;
                    packet >> type;

                    switch ((PacketType)type)
                    {
                    case PacketType::Hello:
                    {
                        if (mClients.size() == 4) // limit connections to 4
                            break;

                        auto found = mClients.find({ sender, port });
                        unsigned short playerId;
                        if (found == mClients.end())
                        {
                            // create player
                            playerId = mGame->addPlayer();
                            mClients.insert({ { sender, port } , playerId });

                            // send player id to client
                            packet.clear();
                            packet << sf::Uint32(PacketType::Welcome) << playerId;
                            status = mSocket.send(packet, sender, port);
                            if (status != sf::Socket::Done) {
                                std::cout << "Welcome send error. Status " << std::to_string((int)status) << std::endl;
                            }
                        }
                        /*else
                        {
                            playerId = found->second;
                        }*/
                        break;
                    }
                    case PlayerUpdate:
                    {
                        auto found = mClients.find({ sender, port });
                        //unsigned short playerId;
                        if (found == mClients.end())
                        {
                            break;
                        }

                        auto client = *found;
                        Player* player = mGame->getPlayer(client.second);

                        if (player == nullptr) break;

                        int moveCount;
                        packet >> msg.objectID >> msg.x >> msg.y >> moveCount;

                        for (int i = 0; i < moveCount; i++)
                        {
                            PlayerMessage update;
                            update.type = UpdateType::Move;
                            update.playerID = player->getId();
                            update.objectID = msg.objectID;

                            packet >> update.force.x >> update.force.y >> update.dt >> update.timestamp;
                            
                            bufferMessage(update);
                        }

                        int shotCount;
                        packet >> shotCount;

                        for (int i = 0; i < shotCount; i++)
                        {
                            PlayerMessage update;
                            update.type = UpdateType::Shot;
                            update.playerID = player->getId();

                            packet >> update.timestamp >> update.position.x >> update.position.y >> update.force.x >> update.force.y;

                            bufferMessage(update);
                        }

                        //bufferMessage(update);

                        //packet >> timestamp;
                        //player->mLastUpdate = (sf::milliseconds(timestamp) + sf::seconds(msg.dt)).asMilliseconds();
                        //player->mLastUpdate = lastUpdate;

                        break;
                    }
                    }
                }
            }
        }
}

void Server::NetworkManager::bufferMessage(const PlayerMessage& update)
{
    auto it = std::lower_bound(
        mMessageQueue.begin(),
        mMessageQueue.end(),
        update,
        [](const PlayerMessage& a, const PlayerMessage& b)
        {
            return a.timestamp < b.timestamp;
        });

    // replace if present
    if (it != mMessageQueue.end() && it->timestamp == update.timestamp && it->type == update.type && it->objectID == update.objectID)
    {
        *it = update;
    }
    // insert
    else
    {
        // Insert the new move at the correct position
        mMessageQueue.insert(it, update);
    }

    //mMessageQueue.push_back(message);
}

NetworkManager::PlayerMessage Server::NetworkManager::processMessage()
{
    PlayerMessage update = mMessageQueue.front();
    mMessageQueue.pop_front();
    return update;
}

bool Server::NetworkManager::hasMessages() const
{
    return !mMessageQueue.empty();
}

sf::Packet& Server::operator<<(sf::Packet& packet, const std::bitset<MAX_MOVES>& bitmap)
{
    // Serialize the bitmap as a series of uint8_t
    for (size_t i = 0; i < MAX_MOVES; i += 8) {
        uint8_t byte = 0;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_MOVES; ++bit) {
            if (bitmap.test(i + bit)) {
                byte |= (1 << bit);
            }
        }
        packet << byte;
    }
    return packet;
}

sf::Packet& Server::operator>>(sf::Packet& packet, std::bitset<MAX_MOVES>& bitmap) {
    for (size_t i = 0; i < MAX_MOVES; i += 8) {
        uint8_t byte = 0;
        packet >> byte; // Extract byte from packet
        for (size_t bit = 0; bit < 8 && i + bit < MAX_MOVES; ++bit) {
            if (byte & (1 << bit)) {
                bitmap.set(i + bit); // Sets the bit at position (i + bit)
            }
            else {
                bitmap.reset(i + bit); // Resets the bit at position (i + bit)
            }
        }
    }
    return packet;
}

sf::Packet& Server::operator<<(sf::Packet& packet, const std::bitset<MAX_SHOTS>& bitmap)
{
    // Serialize the bitmap as a series of uint8_t
    for (size_t i = 0; i < MAX_SHOTS; i += 8) {
        uint8_t byte = 0;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_SHOTS; ++bit) {
            if (bitmap.test(i + bit)) {
                byte |= (1 << bit);
            }
        }
        packet << byte;
    }
    return packet;
}

sf::Packet& Server::operator>>(sf::Packet& packet, std::bitset<MAX_SHOTS>& bitmap) {
    for (size_t i = 0; i < MAX_SHOTS; i += 8) {
        uint8_t byte = 0;
        packet >> byte; // Extract byte from packet
        for (size_t bit = 0; bit < 8 && i + bit < MAX_SHOTS; ++bit) {
            if (byte & (1 << bit)) {
                bitmap.set(i + bit); // Sets the bit at position (i + bit)
            }
            else {
                bitmap.reset(i + bit); // Resets the bit at position (i + bit)
            }
        }
    }
    return packet;
}

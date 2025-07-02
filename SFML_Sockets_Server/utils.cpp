#include "utils.h"

sf::Packet& Server::operator<<(sf::Packet& packet, const std::bitset<MAX_MOVES>& bitmap) {
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

// Deserialize the bitmap from an SFML packet
sf::Packet& Server::operator>>(sf::Packet& packet, std::bitset<MAX_MOVES>& bitmap) {
    for (size_t i = 0; i < MAX_MOVES; i += 8) {
        uint8_t byte = 0;
        packet >> byte;
        for (size_t bit = 0; bit < 8 && i + bit < MAX_MOVES; ++bit) {
            if (byte & (1 << bit)) {
                bitmap.set(i + bit);
            }
            else {
                bitmap.reset(i + bit);
            }
        }
    }
    return packet;
}
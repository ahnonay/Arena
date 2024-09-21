#pragma once
/***
 * IDs for the different types of messages exchanged between server and clients in the Lobby classes.
 */

#include <SFML/Network.hpp>

enum class LobbyServerToClientPacketTypes : std::uint8_t {
    StartGame, UpdatePlayersList
};


enum class LobbyClientToServerPacketTypes : std::uint8_t {
    UpdatePlayerName
};
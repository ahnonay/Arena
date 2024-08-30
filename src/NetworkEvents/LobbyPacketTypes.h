#pragma once
/***
 * IDs for the different types of messages exchanged between server and clients in the Lobby classes.
 */

#include <SFML/Network.hpp>

enum class LobbyServerToClientPacketTypes : sf::Uint8 {
    StartGame, UpdatePlayersList
};


enum class LobbyClientToServerPacketTypes : sf::Uint8 {
    UpdatePlayerName
};
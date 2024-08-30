#pragma once

#include <SFML/Graphics.hpp>

/***
 * Like sf::Vertex, but with a depth coordinate. Required for Sprite3 and Tilemap.
 * https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
 */
class Vertex3 {
public:
    Vertex3()
            : position3D(0, 0, 0), color(255, 255, 255), texCoords(0, 0) {
    }

    sf::Vector3f position3D;  //!< 3D position of the vertex. Third coordinate is depth
    sf::Color color;          //!< Color of the vertex
    sf::Vector2f texCoords;   //!< Coordinates of the texture's pixel to tilemap to the vertex
};

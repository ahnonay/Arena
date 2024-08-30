#pragma once

#include <SFML/Graphics.hpp>
#include "../Util.h"
#include "Vertex3.h"

/***
 * A sprite with support for depth testing and setting the object's depth using the getDepth and setDepth functions.
 * Based on sf::Sprite and https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md.
 */
class Sprite3 : public sf::Drawable, public sf::Transformable {
public:
    Sprite3() : texture(nullptr) {}

    void setTexture(const sf::Texture &texture, bool resetRect = false);

    void setTextureRect(const sf::IntRect &rectangle);

    void setDepth(float newDepth);

    float getDepth() const;

    void setColor(const sf::Color &color);

    const sf::Color &getColor() const;

private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

    Vertex3 vertices[4];            //!< Vertices defining the sprite's geometry
    const sf::Texture *texture;     //!< Texture of the sprite
    sf::IntRect textureRect;        //!< Rectangle defining the area of the source texture to display
};
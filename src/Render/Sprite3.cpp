#include "Sprite3.h"
#include "SFML/OpenGL.hpp"

// https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
// Re-implementation of sf::RenderTarget::draw() to allow depth testing
void Sprite3::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (not texture)
        return;

    states.transform *= getTransform();
    states.texture = texture;

    // Apply the transform
    glLoadMatrixf(states.transform.getMatrix());

    // We do the following two calls already in Game.render, but leaving them here in case of future issues
    // Apply the view
    //applyCurrentView(target);

    // Apply the blend mode
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Apply the texture
    sf::Texture::bind(states.texture, sf::CoordinateType::Pixels);

    // Apply the shader
    if (states.shader)
        sf::Shader::bind(states.shader);

    // Setup the pointers to the vertices' components
    const char *data = reinterpret_cast<const char *>(&vertices[0]);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex3), data);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex3), data + 12);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex3), data + 16);

    // Draw the primitives
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Unbind the shader, if any
    if (states.shader)
        sf::Shader::bind(nullptr);
}

// Copied from SFML/Sprite.cpp
void Sprite3::setTexture(const sf::Texture &texture, bool resetRect) {
    // Recompute the texture area if requested, or if there was no valid texture & rect before
    if (resetRect || (!this->texture && (textureRect == sf::IntRect()))) {
        sf::Vector2i size = sf::Vector2i(texture.getSize());
        setTextureRect(sf::IntRect({0, 0}, {size.x, size.y}));
    }

    // Assign the new texture
    this->texture = &texture;
}

// Adapted from SFML/Sprite.cpp
void Sprite3::setTextureRect(const sf::IntRect &rectangle) {
    if (rectangle != textureRect) {
        textureRect = rectangle;

        float width = static_cast<float>(std::abs(textureRect.size.x));
        float height = static_cast<float>(std::abs(textureRect.size.y));
        float depth = getDepth();
        vertices[0].position3D = sf::Vector3f(0, 0, depth);
        vertices[1].position3D = sf::Vector3f(0, height, depth);
        vertices[2].position3D = sf::Vector3f(width, 0, depth);
        vertices[3].position3D = sf::Vector3f(width, height, depth);

        float left = textureRect.position.x;
        float right = left + textureRect.size.x;
        float top = textureRect.position.y;
        float bottom = top + textureRect.size.y;
        vertices[0].texCoords = sf::Vector2f(left, top);
        vertices[1].texCoords = sf::Vector2f(left, bottom);
        vertices[2].texCoords = sf::Vector2f(right, top);
        vertices[3].texCoords = sf::Vector2f(right, bottom);
    }
}

// Copied from SFML/Sprite.cpp
void Sprite3::setColor(const sf::Color &color) {
    // Update the vertices' color
    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;
    vertices[3].color = color;

}

// Copied from SFML/Sprite.cpp
const sf::Color &Sprite3::getColor() const {
    return vertices[0].color;
}

void Sprite3::setDepth(float newDepth) {
    vertices[0].position3D.z = newDepth;
    vertices[1].position3D.z = newDepth;
    vertices[2].position3D.z = newDepth;
    vertices[3].position3D.z = newDepth;
}

float Sprite3::getDepth() const {
    return vertices[0].position3D.z;
}

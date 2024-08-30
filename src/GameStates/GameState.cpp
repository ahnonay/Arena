#include "GameState.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/OpenGL.hpp>


std::shared_ptr<sf::Font> GameState::defaultFont;
std::unique_ptr<IMGUI> GameState::imgui;
std::shared_ptr<sf::RenderWindow> GameState::window;

void GameState::loadStaticResources() {
    // Request type depth buffer
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 0;
    window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1600, 900), "Arena", sf::Style::Default, settings);
    window->setVerticalSyncEnabled(true);
    window->setView(window->getDefaultView());

    // Configure depth functions
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
    // AlphaFunc is needed to ensure the depth buffer works with textures that have transparency.
    // However, only full transparency or full opaqueness are supported
    glAlphaFunc(GL_GREATER, 0.0f);

    defaultFont = std::make_shared<sf::Font>();
    defaultFont->loadFromFile("Data/Mesmerize Sc Sb.otf");

    imgui = std::make_unique<IMGUI>(window, defaultFont, sf::Vector2f(1600.f, 900.f), 50);
}

void GameState::unloadStaticResources() {
    window->close();
    imgui = nullptr;
    defaultFont = nullptr;
    window = nullptr;
}

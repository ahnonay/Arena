#pragma once

#include <memory>
#include "SFML/Graphics.hpp"


/***
 * A class for an immediate-mode-style GUI. That means that calling button(...) immediately renders a button
 * on screen and returns true or false depending on whether that button has just been clicked.
 *
 * In each iteration of the program loop, call prepare(...) first, then call the functions for all the GUI
 * elements you want to display, then call finish().
 *
 * For some GUI elements, a toolTip can also be created by passing the corresponding parameters. Then, an additional
 * text is shown when the element is hovered.
 *
 * All GUI elements are automatically scaled depending on the current window size:
 *  - If the window has size 'canvasSize', everything is drawn as-is. Per default, canvasSize=(1600,900). So when
 *    creating a GUI element, the position etc. is expected to be on a 1600x900 pixel grid.
 *  - If the window size is different from canvasSize, all positions, sizes, and font sizes of GUI elements
 *    are automatically changed to adjust for that (e.g., make buttons larger if the window is larger than 1600x900).
 *    Aspect ratios are preserved in the following way:
 *      - If the new window size has the same aspect ratio as canvasSize, just scale all positions, sizes, etc.
 *      - If the new aspect ratio is wider, add padding to horizontal positions larger than canvasSize.x / 2
 *      - If the new aspect ratio is narrower, add padding to vertical positions larger than canvasSize.y / 2
 */
class IMGUI {
public:
    IMGUI(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font, sf::Vector2f canvasSize, unsigned int defaultCanvasFontSize);

    bool button(int id, float x, float y, const std::string &text, sf::Texture *img = nullptr, float w = -1.f, float h = -1.f, unsigned int fontSize = 0, bool activated = false, const std::string &toolTipText = "", unsigned int toolTipFontSize = 0);

    bool checkBox(int id, float x, float y, const std::string &text, bool isTicked, float size = -1.f, unsigned int fontSize = 0);

    bool textBox(int id, float x, float y, std::string *buffer, int maxChars, bool numOnly = false, float w = -1, float h = -1, unsigned int fontSize = 0);

    void text(float x, float y, const std::string &text, unsigned int fontSize = 0);

    void fillBar(float x, float y, float cur, float max, float w, float h, const sf::Color &color = sf::Color::White, unsigned int toolTipFontSize = 0);

    void toolTip(const std::string &text, unsigned int fontSize = 0);

    void prepare(bool textEntered = false, std::uint32_t textEnteredUnicode = 0);

    void finish();

    void transformXY(float &x, float &y) const;

    float getScale() const { return scale; }

    int getLastHotItem() const { return lastHotItem; }

private:
    void drawToolTip();
    std::string frameToolTipText;
    unsigned int frameToolTipFontSize;

    int hotItem;
    int lastHotItem;
    int activatedItem;
    bool mouseDown;
    bool textEntered;
    std::uint32_t textEnteredUnicode;
    sf::Vector2f curMousePos;

    std::shared_ptr<sf::Font> font;
    std::shared_ptr<sf::RenderWindow> window;
    sf::Vector2f canvasSize;
    unsigned int defaultCanvasFontSize;
    sf::Vector2f curWindowSize;
    float scale;
    float centerPaddingX;
    float centerPaddingY;

    sf::Color normalColor;
    sf::Color hotColor;
    sf::Color activatedColor;
    sf::Color textColor;

    unsigned int getScaledFontSize(unsigned int fontSize) const;
};
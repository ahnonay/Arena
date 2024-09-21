#include "IMGUI.h"
#include "../Util.h"

#include <utility>

IMGUI::IMGUI(std::shared_ptr<sf::RenderWindow> window, std::shared_ptr<sf::Font> font, sf::Vector2f canvasSize, unsigned int defaultCanvasFontSize) :
        font(std::move(font)), window(std::move(window)), canvasSize(canvasSize), defaultCanvasFontSize(defaultCanvasFontSize) {
    normalColor = sf::Color(150, 30, 40);
    hotColor = sf::Color(190, 30, 40);
    activatedColor = sf::Color(220, 30, 40);
    textColor = sf::Color(255, 255, 255);
    textEntered = false;
    textEnteredUnicode = 0;

    mouseDown = false;
    curMousePos.x = 0;
    curMousePos.y = 0;
    hotItem = 0;
    lastHotItem = 0;
    activatedItem = 0;

    scale = 1.f;
    centerPaddingX = 0.f;
    centerPaddingY = 0.f;
}

void IMGUI::prepare(bool textEntered, std::uint32_t textEnteredUnicode) {
    // textEntered
    this->textEntered = textEntered;
    this->textEnteredUnicode = textEnteredUnicode;

    // Mouse
    auto mousePosi = sf::Mouse::getPosition(*this->window);
    this->curMousePos.x = mousePosi.x;
    this->curMousePos.y = mousePosi.y;
    mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    hotItem = 0;

    // Canvas and scale
    curWindowSize = this->window->getView().getSize();
    if (canvasSize == curWindowSize) {
        scale = 1.f;
        centerPaddingX = 0.f;
        centerPaddingY = 0.f;
    } else {
        float canvasAspectRatio = canvasSize.x / canvasSize.y;
        float curAspectRatio = curWindowSize.x / curWindowSize.y;
        if (curAspectRatio >= canvasAspectRatio) {
            // More space in horizontal direction
            scale = curWindowSize.y / canvasSize.y;
            centerPaddingX = curWindowSize.x - scale * canvasSize.x;
            centerPaddingY = 0.f;
        } else {
            // More space in vertical direction
            scale = curWindowSize.x / canvasSize.x;
            centerPaddingX = 0.f;
            centerPaddingY = curWindowSize.y - scale * canvasSize.y;
        }
    }

    // ToolTip
    frameToolTipText.clear();
    frameToolTipFontSize = 0;
}

void IMGUI::finish() {
    if (mouseDown == 0)
        activatedItem = 0;
    else if (activatedItem == 0)
        activatedItem = -1;
    lastHotItem = hotItem;
    drawToolTip();
}

bool IMGUI::button(int id, float x, float y, const std::string &text, sf::Texture *img, float w, float h,
                   unsigned int fontSize, bool activated, const std::string &toolTipText, unsigned int toolTipFontSize) {
    unsigned int scaledFontSize = getScaledFontSize(fontSize);
    sf::Text textDraw(*font, text, scaledFontSize);
    sf::FloatRect textBounds = textDraw.getGlobalBounds();
    if (w == -1 || h == -1) {
        if (img != nullptr) {
            h = img->getSize().y * scale;
            w = img->getSize().x * scale;
        } else {
            h = textBounds.size.y + scaledFontSize;
            w = textBounds.size.x + 2 * scaledFontSize;
        }
    } else {
        w *= scale;
        h *= scale;
    }

    transformXY(x, y);
    sf::FloatRect buttonRect({x, y}, {w, h});

    if (buttonRect.contains(curMousePos)) {
        hotItem = id;
        if (activatedItem == 0 && mouseDown)
            activatedItem = id;
    }

    // sf::Text behaves weirdly with some inbuilt padding
    textDraw.setPosition({
            buttonRect.position.x + ((buttonRect.size.x - textBounds.size.x) / 2.f) - textBounds.position.x,
            buttonRect.position.y + ((buttonRect.size.y - textBounds.size.y) / 2.f) - textBounds.position.y});
    textDraw.setFillColor(textColor);

    if (img == nullptr) {
        sf::RectangleShape rectDraw(buttonRect.size);
        rectDraw.setPosition(buttonRect.position);
        if (hotItem == id or activated) {
            if (activatedItem == id or activated)
                rectDraw.setFillColor(activatedColor);
            else
                rectDraw.setFillColor(hotColor);
        } else
            rectDraw.setFillColor(normalColor);
        window->draw(rectDraw);
    } else {
        sf::Sprite sprite(*img);
        sprite.setPosition(buttonRect.position);
        sprite.setScale({buttonRect.size.x / img->getSize().x, buttonRect.size.y / img->getSize().y});
        if (!activated and activatedItem != id) {
            if (hotItem == id)
                sprite.setColor(sf::Color(200,200,200));
            else
                sprite.setColor(sf::Color(150,150,150));
        }
        window->draw(sprite);
    }
    window->draw(textDraw);

    if (!toolTipText.empty() and hotItem == id)
        toolTip(toolTipText, toolTipFontSize);

    return mouseDown == 0 && hotItem == id && activatedItem == id;
}

unsigned int IMGUI::getScaledFontSize(unsigned int fontSize) const {
    if (fontSize == 0)
        fontSize = defaultCanvasFontSize;
    unsigned int scaledFontSize = fontSize * scale;
    return scaledFontSize;
}

bool IMGUI::textBox(int id, float x, float y, std::string *buffer, int maxChars, bool numOnly, float w, float h,
                    unsigned int fontSize) {
    unsigned int scaledFontSize = getScaledFontSize(fontSize);

    sf::Text dummyTextDraw(*font, "0", scaledFontSize);
    sf::FloatRect dummyTextBounds = dummyTextDraw.getGlobalBounds();
    if (w == -1 || h == -1) {
        h = dummyTextBounds.size.y + scaledFontSize;
        w = dummyTextBounds.size.x * maxChars + 2 * scaledFontSize;
    } else {
        w *= scale;
        h *= scale;
    }

    transformXY(x, y);
    sf::FloatRect textBoxRect({x, y}, {w, h});

    if (textBoxRect.contains(curMousePos)) {
        hotItem = id;
        if (activatedItem == 0 && mouseDown)
            activatedItem = id;
    }

    sf::Text textDraw(*font, sf::String::fromUtf8(buffer->begin(), buffer->end()), scaledFontSize);
    sf::FloatRect textBounds = textDraw.getGlobalBounds();
    textDraw.setPosition(
            {textBoxRect.position.x + ((textBoxRect.size.x - textBounds.size.x) / 2.f) - textBounds.position.x,
            textBoxRect.position.y + ((textBoxRect.size.y - textBounds.size.y) / 2.f) - textBounds.position.y});
    textDraw.setFillColor(textColor);

    sf::RectangleShape rectDraw(textBoxRect.size);
    rectDraw.setPosition(textBoxRect.position);
    if (hotItem == id || activatedItem == id)
        rectDraw.setFillColor(activatedColor);
    else
        rectDraw.setFillColor(normalColor);
    window->draw(rectDraw);
    window->draw(textDraw);

    auto len = buffer->length();
    bool changed = false;
    if (hotItem == id and textEntered) {
        if (textEnteredUnicode == 8 && len > 0) {  // backspace
            auto curChar = buffer->begin();
            auto nextChar = sf::Utf8::next(curChar, buffer->end());
            while (nextChar != buffer->end()) {
                curChar = nextChar;
                nextChar = sf::Utf8::next(curChar, buffer->end());
            }
            buffer->erase(curChar, nextChar);
            changed = true;
        } else if (sf::Utf8::count(buffer->begin(), buffer->end()) < maxChars && ((numOnly && textEnteredUnicode > 47 && textEnteredUnicode < 58) || (!numOnly && textEnteredUnicode > 31))) {
            sf::Utf8::encode(textEnteredUnicode, std::back_inserter(*buffer));
            changed = true;
        }
    }

    return changed;
}

void IMGUI::text(float x, float y, const std::string &text, unsigned int fontSize) {
    unsigned int scaledFontSize = getScaledFontSize(fontSize);
    sf::Text textDraw(*font, sf::String::fromUtf8(text.begin(), text.end()), scaledFontSize);
    transformXY(x, y);
    textDraw.setPosition({x, y});
    textDraw.setFillColor(textColor);
    window->draw(textDraw);
    //if (!toolTipText.empty() and textDraw.getGlobalBounds().contains(curMousePos))
    //    toolTip(toolTipText, toolTipFontSize);
}

void IMGUI::transformXY(float &x, float &y) const {
    x *= scale;
    if (x >= canvasSize.x * scale / 2.f)
        x += centerPaddingX;
    y *= scale;
    if (y >= canvasSize.y * scale / 2.f)
        y += centerPaddingY;
}

bool IMGUI::checkBox(int id, float x, float y, const std::string &text, bool isTicked, float size, unsigned int fontSize) {
    unsigned int scaledFontSize = getScaledFontSize(fontSize);
    sf::Text textDraw(*font, text, scaledFontSize);
    sf::FloatRect textBounds = textDraw.getGlobalBounds();
    if (size == -1)
        size = textBounds.size.y + scaledFontSize;
    else
        size *= scale;

    transformXY(x, y);
    sf::FloatRect boxRect({x, y}, {size, size});

    if (boxRect.contains(curMousePos)) {
        hotItem = id;
        if (activatedItem == 0 && mouseDown)
            activatedItem = id;
    }

    // sf::Text behaves weirdly with some inbuilt padding
    textDraw.setPosition(
            {boxRect.position.x + 1.3f * boxRect.size.x,
            boxRect.position.y + ((boxRect.size.y - textBounds.size.y) / 2.f) - textBounds.position.y});
    textDraw.setFillColor(textColor);

    sf::RectangleShape rectDraw(boxRect.size);
    rectDraw.setPosition(boxRect.position);
    sf::Color color = normalColor;
    if (hotItem == id) {
        if (activatedItem == id)
            color = activatedColor;
        else
            color = hotColor;
    }
    rectDraw.setOutlineThickness(size / 10.f);
    rectDraw.setOutlineColor(color);
    if (isTicked)
        rectDraw.setFillColor(color);
    else
        rectDraw.setFillColor(sf::Color::Transparent);
    window->draw(rectDraw);
    window->draw(textDraw);

    return mouseDown == 0 && hotItem == id && activatedItem == id;
}

void IMGUI::fillBar(float x, float y, float cur, float max, float w, float h, const sf::Color &color, unsigned int toolTipFontSize) {
    float percentage = cur / max;
    sf::RectangleShape fillBar(sf::Vector2f(percentage * w * scale, h * scale));
    fillBar.setFillColor(color);
    transformXY(x, y);
    fillBar.setPosition({x, y});
    window->draw(fillBar);
    if (toolTipFontSize > 0 and fillBar.getGlobalBounds().contains(curMousePos))
        toolTip(toStr(cur, " / ", max), toolTipFontSize);
}

void IMGUI::toolTip(const std::string &text, unsigned int fontSize) {
    frameToolTipText = text;
    frameToolTipFontSize = fontSize;
}

void IMGUI::drawToolTip() {
    if (frameToolTipFontSize == 0 or frameToolTipText.empty())
        return;
    unsigned int scaledFontSize = getScaledFontSize(frameToolTipFontSize);
    sf::Text textDraw(*font, frameToolTipText, scaledFontSize);
    sf::FloatRect textBounds = textDraw.getGlobalBounds();
    float h = textBounds.size.y + scaledFontSize;
    float w = textBounds.size.x + 2 * scaledFontSize;
    sf::RectangleShape rectDraw(sf::Vector2f(w, h));

    // Default positioning of ToolTips is to the upper right of the cursor
    // But can move this around if we get too close to the window border
    float x = this->curMousePos.x;
    if (x + w >= curWindowSize.x)
        x -= w;
    float y = this->curMousePos.y - h;
    if (y < 0)
        y += h;
    rectDraw.setPosition({x, y});
    rectDraw.setFillColor(normalColor);
    window->draw(rectDraw);

    textDraw.setPosition(
            {x + ((w - textBounds.size.x) / 2.f) - textBounds.position.x,
            y + ((h - textBounds.size.y) / 2.f) - textBounds.position.y});
    textDraw.setFillColor(textColor);
    window->draw(textDraw);
}

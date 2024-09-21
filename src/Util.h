#pragma once

#include <string>
#include <algorithm>
#include <queue>
#include <variant>
#include <array>
#include <cassert>
#include <sstream>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

// Compass directions. Used, e.g., for directions into which a character sprite may be facing.
enum class ORIENTATIONS : std::uint8_t {
    E, N, NE, NW, S, SE, SW, W, NUM_ORIENTATIONS
};

// String trimming functions from https://stackoverflow.com/a/217605
// trim from start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

template <typename T> void toStr_helper(std::stringstream& ss, T t) {
    ss << t;
}

template<typename T, typename... Args> inline void toStr_helper(std::stringstream& ss, T t, Args... args) {
    ss << t;
    toStr_helper(ss, args...);
}

// Concatenate arbitrary data types into a string. So toStr("Hi ", 5, "!") returns "Hi 5!"
template<typename... Args> inline std::string toStr(Args... args) {
    std::stringstream ss;
    toStr_helper(ss, args...);
    return ss.str();
}

// For depth rendering. See https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
inline sf::IntRect getViewportAsIntRect(sf::RenderTarget &target, const sf::View &view) {
    float width = static_cast<float>(target.getSize().x);
    float height = static_cast<float>(target.getSize().y);
    const sf::FloatRect &viewport = view.getViewport();

    return {{
            static_cast<int>(0.5f + width * viewport.position.x),
           static_cast<int>(0.5f + height * viewport.position.y)},
            {static_cast<int>(width * viewport.size.x),
            static_cast<int>(height * viewport.size.y)}};
}

// For depth rendering. See https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
inline void applyCurrentView(sf::RenderTarget &target) {
    // Set the viewport
    sf::IntRect viewport = getViewportAsIntRect(target, target.getView());
    int top = target.getSize().y - (viewport.position.y + viewport.size.y);
    (glViewport(viewport.position.x, top, viewport.size.x, viewport.size.y));

    // Set the projection matrix
    (glMatrixMode(GL_PROJECTION));
    (glLoadMatrixf(target.getView().getTransform().getMatrix()));

    // Go back to model-view mode
    (glMatrixMode(GL_MODELVIEW));
}

// Remove all entries in the given queue.
template <typename T> inline void clearQueue(std::queue<T> &q) {
    while (!q.empty())
        q.pop();
}

// Create an instance of the i-th type of the given std::variant. See https://www.reddit.com/r/cpp/comments/f8cbzs/creating_stdvariant_based_on_index_at_runtime/,  https://gcc.godbolt.org/z/yRi7ND
template <typename... Ts> [[nodiscard]] std::variant<Ts...> expand_type(std::size_t i, std::variant<Ts...> const&) {
    assert(i < sizeof...(Ts));
    static constexpr auto table = std::array{ +[](){ return std::variant<Ts...>{Ts{ }}; }...  };
    return table[i]();
}
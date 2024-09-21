#pragma once
/***
 * Various utility functions for fixed-point numbers.
 */
#include "Util.h"
#include <SFML/Graphics.hpp>
#include <fpm/fixed.hpp>
#include <fpm/math.hpp>


typedef fpm::fixed_16_16 FPMNum;
typedef fpm::fixed_24_8 FPMNum24;
typedef sf::Vector2<FPMNum> FPMVector2;
typedef sf::Rect<FPMNum> FPMRect;

// Return a vector of unit length pointing in the given direction.
inline FPMVector2 getDirectionVector(ORIENTATIONS orientations) {
#define SQRT05 0.707106781
    switch (orientations) {
        case ORIENTATIONS::E:
            return {FPMNum {SQRT05}, FPMNum {-SQRT05}};
        case ORIENTATIONS::N:
            return {FPMNum {-SQRT05}, FPMNum {-SQRT05}};
        case ORIENTATIONS::NE:
            return {FPMNum {0}, FPMNum {-1}};
        case ORIENTATIONS::NW:
            return {FPMNum {-1}, FPMNum {0}};
        case ORIENTATIONS::S:
            return {FPMNum {SQRT05}, FPMNum {SQRT05}};
        case ORIENTATIONS::SE:
            return {FPMNum {1}, FPMNum {0}};
        case ORIENTATIONS::SW:
            return {FPMNum {0}, FPMNum {1}};
        case ORIENTATIONS::W:
            return {FPMNum {-SQRT05}, FPMNum {SQRT05}};
        default:
            return {FPMNum {0}, FPMNum {0}};
    }
}

// TODO Extend sf::Vector and override its member functions instead?

inline FPMVector2 fromAngle(FPMNum angleInRadians) { return {fpm::cos(angleInRadians), fpm::sin(angleInRadians)}; }

inline bool isNull(const FPMVector2& a) { return a.x == FPMNum(0) && a.y == FPMNum(0); }

inline void setNull(FPMVector2& a) { a.x = FPMNum(0); a.y = FPMNum(0); }

inline FPMNum getLength(const FPMVector2& a) {
    auto lSq = a.lengthSquared();
    if (lSq == FPMNum(0))
        return FPMNum(0);
    else
        return fpm::sqrt(lSq);
}

inline FPMNum getAngle(const FPMVector2& a) { return fpm::atan2(a.y, a.x); }

inline void normalize(FPMVector2& a) {
    auto l = getLength(a);
    if (l == FPMNum(0))
        setNull(a);
    else {
        a.x /= l;
        a.y /= l;
    }
}

inline void setLength(FPMVector2& a, FPMNum l) {
    if (l < FPMNum(0))
        throw std::runtime_error("Cannot set length of vector to negative value");

    normalize(a);
    a *= l;
}

// Clip vectors with a length below 'low' to 0. Additionally, clip vectors to have at most length 'high'.
inline void clipLength(FPMVector2& a, FPMNum high, FPMNum low = FPMNum(0)) {
    if (isNull(a) || high < FPMNum(0))
        return;
    auto curL = getLength(a);
    if (curL < low)
        setNull(a);
    else if (curL > high) {
        a.x /= curL;
        a.y /= curL;
        a *= high;
    }
}

inline void rotate(FPMVector2& a, FPMNum angleInRadians) {
    FPMNum xt = (a.x * fpm::cos(angleInRadians)) - (a.y * fpm::sin(angleInRadians));
    a.y = (a.y * fpm::cos(angleInRadians)) + (a.x * fpm::sin(angleInRadians));
    a.x = xt;
}
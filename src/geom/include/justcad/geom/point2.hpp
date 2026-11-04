#pragma once

#include "justcad/geom/vec2.hpp"

namespace justcad::geom {

/// Ponto 2D (posição). Separado de Vec2 para manter a semântica afim:
/// Ponto - Ponto = Vetor, Ponto + Vetor = Ponto, Ponto + Ponto não existe.
template <class T>
struct Point2 {
    T x{};
    T y{};

    constexpr Vec2<T> operator-(const Point2& o) const { return {x - o.x, y - o.y}; }
    constexpr Point2 operator+(const Vec2<T>& v) const { return {x + v.x, y + v.y}; }
    constexpr Point2 operator-(const Vec2<T>& v) const { return {x - v.x, y - v.y}; }
    constexpr Point2& operator+=(const Vec2<T>& v) { x += v.x; y += v.y; return *this; }

    /// Igualdade exata, bit a bit. Para comparar com tolerância use Kernel::equal.
    constexpr bool operator==(const Point2&) const = default;
};

/// Interpolação afim: (1 - t) * a + t * b.
template <class T>
[[nodiscard]] constexpr Point2<T> lerp(const Point2<T>& a, const Point2<T>& b, const T& t) {
    return a + (b - a) * t;
}

template <class T>
[[nodiscard]] constexpr T squaredDistance(const Point2<T>& a, const Point2<T>& b) {
    return squaredLength(b - a);
}

}  // namespace justcad::geom

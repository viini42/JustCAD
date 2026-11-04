#pragma once

#include <cmath>

namespace justcad::geom {

/// Vetor livre 2D (direção e magnitude).
template <class T>
struct Vec2 {
    T x{};
    T y{};

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2 operator*(const T& s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(const T& s) const { return {x / s, y / s}; }
    constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(const T& s) { x *= s; y *= s; return *this; }

    constexpr bool operator==(const Vec2&) const = default;
};

template <class T>
constexpr Vec2<T> operator*(const T& s, const Vec2<T>& v) { return v * s; }

template <class T>
[[nodiscard]] constexpr T dot(const Vec2<T>& a, const Vec2<T>& b) { return a.x * b.x + a.y * b.y; }

/// Componente z do produto vetorial (área orientada do paralelogramo).
template <class T>
[[nodiscard]] constexpr T cross(const Vec2<T>& a, const Vec2<T>& b) { return a.x * b.y - a.y * b.x; }

template <class T>
[[nodiscard]] constexpr T squaredLength(const Vec2<T>& v) { return dot(v, v); }

/// Só faz sentido para escalares com raiz quadrada (não para racionais exatos).
template <class T>
[[nodiscard]] T length(const Vec2<T>& v) { using std::sqrt; return sqrt(squaredLength(v)); }

/// Vetor perpendicular (rotação de +90 graus).
template <class T>
[[nodiscard]] constexpr Vec2<T> perp(const Vec2<T>& v) { return {-v.y, v.x}; }

}  // namespace justcad::geom

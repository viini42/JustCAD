#pragma once

#include <cmath>

#include "justcad/geom/kernel.hpp"
#include "justcad/geom/point2.hpp"
#include "justcad/geom/vec2.hpp"

namespace justcad::geom {

/// Kernel em ponto flutuante com uma tolerância global de distância.
///
/// A tolerância tem significado geométrico (uma distância no plano): dois
/// pontos são iguais se estão a no máximo `tolerance()` um do outro, e três
/// pontos são colineares se c está a no máximo `tolerance()` da reta a->b.
class DoubleKernel {
public:
    using Scalar = double;
    using Point2 = geom::Point2<double>;
    using Vec2 = geom::Vec2<double>;

    static constexpr double kDefaultTolerance = 1e-9;

    [[nodiscard]] static double tolerance() noexcept { return s_tolerance; }
    static void setTolerance(double eps) noexcept { s_tolerance = std::abs(eps); }

    [[nodiscard]] static Sign sign(double v) noexcept {
        if (std::abs(v) <= s_tolerance) return Sign::Zero;
        return v > 0.0 ? Sign::Positive : Sign::Negative;
    }

    [[nodiscard]] static Sign orientation(const Point2& a, const Point2& b, const Point2& c) noexcept {
        const Vec2 ab = b - a;
        const double len = length(ab);
        if (len <= s_tolerance) return Sign::Zero;  // a e b coincidem: degenerado
        // cross / |ab| = distância com sinal de c até a reta a->b.
        return sign(cross(ab, c - a) / len);
    }

    [[nodiscard]] static bool equal(const Point2& a, const Point2& b) noexcept {
        return squaredDistance(a, b) <= s_tolerance * s_tolerance;
    }

    [[nodiscard]] static Sign compareXY(const Point2& a, const Point2& b) noexcept {
        if (equal(a, b)) return Sign::Zero;
        const Sign sx = sign(a.x - b.x);
        return sx != Sign::Zero ? sx : sign(a.y - b.y);
    }

    [[nodiscard]] static bool onSegment(const Point2& a, const Point2& b, const Point2& c) noexcept {
        if (equal(a, c) || equal(b, c)) return true;
        if (orientation(a, b, c) != Sign::Zero) return false;
        // Colinear: está entre a e b se a projeção cai em [0, |ab|].
        const Vec2 ab = b - a;
        const double t = dot(c - a, ab);
        return t >= 0.0 && t <= squaredLength(ab);
    }

private:
    inline static double s_tolerance = kDefaultTolerance;
};

static_assert(Kernel<DoubleKernel>);

/// Restaura a tolerância anterior ao sair do escopo (útil em testes).
class ScopedTolerance {
public:
    explicit ScopedTolerance(double eps) noexcept : m_previous(DoubleKernel::tolerance()) {
        DoubleKernel::setTolerance(eps);
    }
    ~ScopedTolerance() { DoubleKernel::setTolerance(m_previous); }
    ScopedTolerance(const ScopedTolerance&) = delete;
    ScopedTolerance& operator=(const ScopedTolerance&) = delete;

private:
    double m_previous;
};

}  // namespace justcad::geom

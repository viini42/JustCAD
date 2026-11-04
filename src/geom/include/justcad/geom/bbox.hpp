#pragma once

#include <algorithm>
#include <limits>

#include "justcad/geom/point2.hpp"

namespace justcad::geom {

/// Caixa alinhada aos eixos. Começa vazia (min > max) e cresce com expand().
template <class T>
struct BBox {
    Point2<T> min{std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};
    Point2<T> max{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest()};

    [[nodiscard]] static constexpr BBox of(const Point2<T>& a, const Point2<T>& b) {
        BBox box;
        box.expand(a);
        box.expand(b);
        return box;
    }

    [[nodiscard]] constexpr bool empty() const { return min.x > max.x || min.y > max.y; }

    constexpr void expand(const Point2<T>& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
    }

    constexpr void expand(const BBox& o) {
        if (o.empty()) return;
        expand(o.min);
        expand(o.max);
    }

    /// Engorda a caixa em `margin` para todos os lados (útil com tolerâncias).
    [[nodiscard]] constexpr BBox inflated(const T& margin) const {
        if (empty()) return *this;
        return {{min.x - margin, min.y - margin}, {max.x + margin, max.y + margin}};
    }

    [[nodiscard]] constexpr bool contains(const Point2<T>& p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }

    [[nodiscard]] constexpr bool intersects(const BBox& o) const {
        if (empty() || o.empty()) return false;
        return min.x <= o.max.x && o.min.x <= max.x && min.y <= o.max.y && o.min.y <= max.y;
    }

    [[nodiscard]] constexpr T width() const { return empty() ? T{} : max.x - min.x; }
    [[nodiscard]] constexpr T height() const { return empty() ? T{} : max.y - min.y; }
};

}  // namespace justcad::geom

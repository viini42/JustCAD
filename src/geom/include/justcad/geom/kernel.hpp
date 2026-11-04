#pragma once

#include <concepts>

#include "justcad/geom/sign.hpp"

namespace justcad::geom {

/**
 * Numeric kernel: the scalar type plus the geometric *predicates*.
 *
 * Upper layers are templated on a kernel so that `double` with a tolerance can
 * later be replaced by exact (or filtered) arithmetic just by switching types.
 * For that to hold, every topological decision (side of a line, coincident
 * points, ordering) must go through these predicates, never through ad hoc
 * epsilon comparisons.
 *
 * Constructions (intersection points, point at t) stay outside the kernel: with
 * exact arithmetic they can be expensive or not representable.
 *
 * - SignOf(s): sign of a scalar (ZERO within the tolerance, if any).
 * - Orientation(a, b, c): POSITIVE if c is left of a->b, NEGATIVE if right, ZERO if collinear.
 * - Equal(a, b): coincident points.
 * - CompareXY(a, b): lexicographic order, x first.
 * - OnSegment(a, b, c): c lies on the closed segment [a, b].
 */
template <typename K>
concept kernel = requires {
    typename K::Scalar;
    typename K::Point2;
    typename K::Vec2;
} && requires(const typename K::Point2& a, const typename K::Point2& b,
              const typename K::Point2& c, const typename K::Scalar& s) {
    { K::SignOf(s) } -> std::same_as<Sign>;
    { K::Orientation(a, b, c) } -> std::same_as<Sign>;
    { K::Equal(a, b) } -> std::same_as<bool>;
    { K::CompareXY(a, b) } -> std::same_as<Sign>;
    { K::OnSegment(a, b, c) } -> std::same_as<bool>;
};

}  // namespace justcad::geom

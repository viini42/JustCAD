#include "geom/double_kernel.hpp"

#include "core/assert.hpp"
#include "geom/kernel.hpp"

#include <cmath>

static_assert(jcad::geom::kernel<jcad::geom::DoubleKernel>);

double jcad::geom::DoubleKernel::s_tolerance{ DEFAULT_TOLERANCE };

double jcad::geom::DoubleKernel::Tolerance() noexcept
{
  return s_tolerance;
}

void jcad::geom::DoubleKernel::SetTolerance(double tolerance)
{
  JCAD_ASSERT(tolerance >= 0.0, "tolerance must be non-negative, got {}", tolerance);
  s_tolerance = tolerance;
}

jcad::geom::Sign jcad::geom::DoubleKernel::SignOf(double value) noexcept
{
  if (std::abs(value) <= s_tolerance)
    return Sign::ZERO;
  return value > 0.0 ? Sign::POSITIVE : Sign::NEGATIVE;
}

jcad::geom::Sign jcad::geom::DoubleKernel::Orientation(Point2 a, Point2 b, Point2 c) noexcept
{
  const Vec2 ab{ b - a };
  const double length{ Length(ab) };
  if (length <= s_tolerance)
    return Sign::ZERO;
  // Dividing by |ab| turns the cross product into the signed distance from c to
  // the line, so the tolerance does not depend on the length of the segment.
  return SignOf(Cross(ab, c - a) / length);
}

bool jcad::geom::DoubleKernel::Equal(Point2 a, Point2 b) noexcept
{
  return SquaredDistance(a, b) <= s_tolerance * s_tolerance;
}

jcad::geom::Sign jcad::geom::DoubleKernel::CompareXY(Point2 a, Point2 b) noexcept
{
  if (Equal(a, b))
    return Sign::ZERO;
  const Sign sign_x{ SignOf(a.x - b.x) };
  return sign_x != Sign::ZERO ? sign_x : SignOf(a.y - b.y);
}

bool jcad::geom::DoubleKernel::OnSegment(Point2 a, Point2 b, Point2 c) noexcept
{
  if (Equal(a, c) || Equal(b, c))
    return true;
  if (Orientation(a, b, c) != Sign::ZERO)
    return false;
  const Vec2 ab{ b - a };
  const double projection{ Dot(c - a, ab) };
  return projection >= 0.0 && projection <= SquaredLength(ab);
}

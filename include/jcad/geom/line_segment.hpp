#pragma once

#include "geom/point2.hpp"

namespace jcad::geom
{

  struct LineSegment
  {
    Point2<double> start{};
    Point2<double> end{};
  };

} // namespace jcad::geom

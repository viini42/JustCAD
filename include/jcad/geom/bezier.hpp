#pragma once

#include "geom/point2.hpp"

#include <vector>

namespace jcad::geom
{

  /** Bézier curve of arbitrary degree: control_points.size() - 1. */
  struct Bezier
  {
    std::vector<Point2<double>> control_points;
  };

} // namespace jcad::geom

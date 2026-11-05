#pragma once

#include "geom/point2.hpp"

namespace jcad::geom
{

  /**
   * Arc of the ellipse centered at `center` with semi-axes `radius_x` and `radius_y`,
   * the x axis turned by `rotation`. It runs from the parametric angle `start_angle`
   * over `sweep` radians (counter-clockwise when positive). A circular arc has equal
   * radii.
   */
  struct EllipticArc
  {
    Point2<double> center{};
    double radius_x{ 1.0 };
    double radius_y{ 1.0 };
    double rotation{ 0.0 };
    double start_angle{ 0.0 };
    double sweep{ 0.0 };
  };

} // namespace jcad::geom

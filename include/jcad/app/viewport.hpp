#pragma once

#include "geom/point2.hpp"
#include "geom/vec2.hpp"

namespace jcad::app
{

  /**
   * Maps world coordinates (y up) to screen pixels (y down) with a pan and a uniform
   * zoom. The world point `Center()` sits at the middle of the screen.
   */
  class Viewport
  {
  public:
    using Point2 = geom::Point2<double>;
    using Vec2 = geom::Vec2<double>;

    static constexpr double MIN_SCALE{ 1e-3 };
    static constexpr double MAX_SCALE{ 1e6 };

    void SetScreenSize(Vec2 size);
    [[nodiscard]] Vec2 ScreenSize() const noexcept;

    [[nodiscard]] Point2 Center() const noexcept;
    void SetCenter(Point2 center);

    /** Pixels per world unit. */
    [[nodiscard]] double Scale() const noexcept;
    void SetScale(double scale);

    [[nodiscard]] Point2 WorldToScreen(Point2 world) const;
    [[nodiscard]] Point2 ScreenToWorld(Point2 screen) const;

    /** Moves the view so the world follows a mouse drag of `screenDelta` pixels. */
    void Pan(Vec2 screenDelta);

    /** Zooms by `factor`, keeping the world point under `screen` fixed. */
    void ZoomAt(Point2 screen, double factor);

  private:
    Vec2 m_screen_size{ 800.0, 600.0 };
    Point2 m_center{};
    double m_scale{ 50.0 };
  };

} // namespace jcad::app

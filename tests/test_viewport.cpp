#include "app/viewport.hpp"

#include <gtest/gtest.h>

using jcad::app::Viewport;
using Point2 = Viewport::Point2;
using Vec2 = Viewport::Vec2;

namespace
{

  void ExpectNear(Point2 actual, Point2 expected)
  {
    EXPECT_NEAR(actual.x, expected.x, 1e-9);
    EXPECT_NEAR(actual.y, expected.y, 1e-9);
  }

} // namespace

TEST(Viewport, WorldToScreenCenterMapsToMiddleWithYDown)
{
  Viewport viewport;
  viewport.SetScreenSize(Vec2{ 200.0, 100.0 });
  viewport.SetScale(10.0);
  ExpectNear(viewport.WorldToScreen(Point2{ 0.0, 0.0 }), Point2{ 100.0, 50.0 });
  ExpectNear(viewport.WorldToScreen(Point2{ 1.0, 1.0 }), Point2{ 110.0, 40.0 });
}

TEST(Viewport, ScreenToWorldInvertsWorldToScreen)
{
  Viewport viewport;
  viewport.SetCenter(Point2{ 3.0, -2.0 });
  viewport.SetScale(7.5);
  const Point2 world{ 1.25, 4.5 };
  ExpectNear(viewport.ScreenToWorld(viewport.WorldToScreen(world)), world);
}

TEST(Viewport, PanMovesWorldWithTheMouse)
{
  Viewport viewport;
  const Point2 world{ 1.0, 1.0 };
  const Point2 before{ viewport.WorldToScreen(world) };
  viewport.Pan(Vec2{ 15.0, -5.0 });
  ExpectNear(viewport.WorldToScreen(world), Point2{ before.x + 15.0, before.y - 5.0 });
}

TEST(Viewport, ZoomAtKeepsPointUnderCursorFixed)
{
  Viewport viewport;
  const Point2 cursor{ 123.0, 45.0 };
  const Point2 anchor{ viewport.ScreenToWorld(cursor) };
  viewport.ZoomAt(cursor, 2.5);
  EXPECT_DOUBLE_EQ(viewport.Scale(), 125.0);
  ExpectNear(viewport.ScreenToWorld(cursor), anchor);
}

TEST(Viewport, SetScaleClampsToLimits)
{
  Viewport viewport;
  viewport.SetScale(1e12);
  EXPECT_DOUBLE_EQ(viewport.Scale(), Viewport::MAX_SCALE);
}

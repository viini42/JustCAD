#include "app/viewport.hpp"

#include "core/assert.hpp"

#include <algorithm>

void jcad::app::Viewport::SetScreenSize(Vec2 size)
{
  m_screen_size = size;
}

jcad::app::Viewport::Vec2 jcad::app::Viewport::ScreenSize() const noexcept
{
  return m_screen_size;
}

jcad::app::Viewport::Point2 jcad::app::Viewport::Center() const noexcept
{
  return m_center;
}

void jcad::app::Viewport::SetCenter(Point2 center)
{
  m_center = center;
}

double jcad::app::Viewport::Scale() const noexcept
{
  return m_scale;
}

void jcad::app::Viewport::SetScale(double scale)
{
  JCAD_ASSERT(scale > 0.0, "scale must be positive, got {}", scale);
  m_scale = std::clamp(scale, MIN_SCALE, MAX_SCALE);
}

jcad::app::Viewport::Point2 jcad::app::Viewport::WorldToScreen(Point2 world) const
{
  const Vec2 offset{ (world - m_center) * m_scale };
  return Point2{ m_screen_size.x / 2.0 + offset.x, m_screen_size.y / 2.0 - offset.y };
}

jcad::app::Viewport::Point2 jcad::app::Viewport::ScreenToWorld(Point2 screen) const
{
  const Vec2 offset{ screen.x - m_screen_size.x / 2.0, m_screen_size.y / 2.0 - screen.y };
  return m_center + offset / m_scale;
}

void jcad::app::Viewport::Pan(Vec2 screenDelta)
{
  m_center += Vec2{ -screenDelta.x, screenDelta.y } / m_scale;
}

void jcad::app::Viewport::ZoomAt(Point2 screen, double factor)
{
  const Point2 anchor{ ScreenToWorld(screen) };
  SetScale(m_scale * factor);
  // Shift the center so that `anchor` maps back to `screen`.
  m_center += anchor - ScreenToWorld(screen);
}

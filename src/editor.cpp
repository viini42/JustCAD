#include "app/editor.hpp"

#include "core/assert.hpp"
#include "topo/topology.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{

  using Point2 = jcad::app::Editor::Point2;
  using Kernel = jcad::model::GeometricModel::Kernel;

  double RoundTo(double value, double step)
  {
    return std::round(value / step) * step;
  }

} // namespace

const jcad::model::GeometricModel& jcad::app::Editor::Model() const noexcept
{
  return m_model;
}

jcad::app::Tool jcad::app::Editor::CurrentTool() const noexcept
{
  return m_tool;
}

void jcad::app::Editor::SetTool(Tool tool)
{
  m_tool = tool;
  m_pending.clear();
}

int jcad::app::Editor::PolygonSides() const noexcept
{
  return m_polygon_sides;
}

void jcad::app::Editor::SetPolygonSides(int sides)
{
  m_polygon_sides = std::max(sides, 3);
}

double jcad::app::Editor::GridStep() const noexcept
{
  return m_grid_step;
}

void jcad::app::Editor::SetGridStep(double step)
{
  m_grid_step = std::max(step, 0.0);
}

void jcad::app::Editor::SetPickRadius(double radius)
{
  m_pick_radius = radius;
}

jcad::app::Editor::Point2 jcad::app::Editor::Snap(Point2 world) const
{
  if (const std::optional<topo::VertexId> vertex{ NearestVertex(world) })
    return m_model.PointOf(*vertex);
  if (m_grid_step <= 0.0)
    return world;
  return Point2{ RoundTo(world.x, m_grid_step), RoundTo(world.y, m_grid_step) };
}

void jcad::app::Editor::Click(Point2 world)
{
  if (m_tool == Tool::SELECT)
  {
    Select(world);
    return;
  }
  const Point2 point{ Snap(world) };
  if (m_tool == Tool::POLYLINE)
  {
    ClickPolyline(point);
    return;
  }
  if (m_tool == Tool::BEZIER)
  {
    m_pending.push_back(point);
    return;
  }
  if (m_pending.size() + 1 < ClicksNeeded())
  {
    m_pending.push_back(point);
    return;
  }
  Complete(point);
}

void jcad::app::Editor::Hover(Point2 world)
{
  m_hover = Snap(world);
}

void jcad::app::Editor::Finish()
{
  if (m_tool == Tool::POLYLINE && m_pending.size() >= 2)
  {
    m_model.AddPolyline(m_pending, false);
    m_selection = Selection{};
  }
  if (m_tool == Tool::BEZIER && m_pending.size() >= 2)
  {
    m_model.AddCurve(geom::Bezier{ m_pending });
    m_selection = Selection{};
  }
  m_pending.clear();
}

void jcad::app::Editor::Cancel()
{
  m_pending.clear();
  m_selection = Selection{};
}

void jcad::app::Editor::DeleteSelection()
{
  if (m_selection.vertex.Valid())
    m_model.RemoveVertex(m_selection.vertex);
  else if (m_selection.edge.Valid())
    m_model.RemoveEdge(m_selection.edge);
  m_selection = Selection{};
}

void jcad::app::Editor::Undo()
{
  // Undo first drops a shape in progress, like Cancel.
  if (!m_pending.empty())
  {
    m_pending.clear();
    return;
  }
  if (m_model.CanUndo())
    m_model.Undo();
  m_selection = Selection{};
}

void jcad::app::Editor::Redo()
{
  if (m_model.CanRedo())
    m_model.Redo();
  m_selection = Selection{};
}

const jcad::app::Editor::Selection& jcad::app::Editor::GetSelection() const noexcept
{
  return m_selection;
}

const std::vector<jcad::app::Editor::Point2>& jcad::app::Editor::PendingPoints() const noexcept
{
  return m_pending;
}

jcad::app::Editor::PreviewShape jcad::app::Editor::Preview() const
{
  if (m_pending.empty() || !m_hover)
    return PreviewShape{};
  const Point2 first{ m_pending.front() };
  const Point2 hover{ *m_hover };
  switch (m_tool)
  {
  case Tool::RECTANGLE:
    return PreviewShape{ { first, Point2{ hover.x, first.y }, hover, Point2{ first.x, hover.y } },
                         true };
  case Tool::POLYGON:
    return PreviewShape{ RegularPolygonPoints(first, hover), true };
  case Tool::CIRCLE:
  case Tool::ELLIPSE:
  case Tool::ARC:
  case Tool::BEZIER:
  {
    const std::optional<geom::Curve> curve{ ToolCurve(hover) };
    if (!curve)
      return PreviewShape{ { first, hover }, false };
    return PreviewShape{ geom::Tessellate(*curve, m_pick_radius / 4.0), false };
  }
  default:
  {
    PreviewShape shape{ m_pending, false };
    shape.points.push_back(hover);
    return shape;
  }
  }
}

void jcad::app::Editor::Select(Point2 world)
{
  m_selection = Selection{};
  if (const std::optional<topo::VertexId> vertex{ NearestVertex(world) })
    m_selection.vertex = *vertex;
  else if (const std::optional<topo::EdgeId> edge{ NearestEdge(world) })
    m_selection.edge = *edge;
}

void jcad::app::Editor::ClickPolyline(Point2 point)
{
  const bool closes{ m_pending.size() >= 3 && Kernel::Equal(point, m_pending.front()) };
  if (!closes)
  {
    m_pending.push_back(point);
    return;
  }
  m_model.AddPolyline(m_pending, true);
  m_pending.clear();
  m_selection = Selection{};
}

void jcad::app::Editor::Complete(Point2 point)
{
  switch (m_tool)
  {
  case Tool::POINT:
    static_cast<void>(m_model.AddPoint(point));
    break;
  case Tool::LINE:
    m_model.AddLine(m_pending.front(), point);
    break;
  case Tool::RECTANGLE:
    m_model.AddRectangle(m_pending.front(), point);
    break;
  case Tool::POLYGON:
    m_model.AddPolyline(RegularPolygonPoints(m_pending.front(), point), true);
    break;
  case Tool::CIRCLE:
  case Tool::ELLIPSE:
  case Tool::ARC:
    if (const std::optional<geom::Curve> curve{ ToolCurve(point) })
      m_model.AddCurve(*curve);
    break;
  default:
    JCAD_ASSERT(false, "tool does not complete on a click");
  }
  m_pending.clear();
  m_selection = Selection{};
}

std::size_t jcad::app::Editor::ClicksNeeded() const noexcept
{
  if (m_tool == Tool::POINT)
    return 1U;
  return m_tool == Tool::ARC ? 3U : 2U;
}

std::optional<jcad::topo::VertexId> jcad::app::Editor::NearestVertex(Point2 world) const
{
  std::optional<topo::VertexId> best;
  double best_distance{ m_pick_radius };
  for (const topo::VertexId vertex : m_model.GetTopology().VertexIds())
  {
    const double distance{ Length(m_model.PointOf(vertex) - world) };
    if (distance <= best_distance)
    {
      best = vertex;
      best_distance = distance;
    }
  }
  return best;
}

std::optional<jcad::topo::EdgeId> jcad::app::Editor::NearestEdge(Point2 world) const
{
  std::optional<topo::EdgeId> best;
  double best_distance{ m_pick_radius };
  for (const model::GeometricModel::EdgeCurve& edge : m_model.EdgeCurves())
  {
    const double t{ geom::ClosestParam(edge.curve, world) };
    const double distance{ Length(geom::Eval(edge.curve, t) - world) };
    if (distance <= best_distance)
    {
      best = edge.edge;
      best_distance = distance;
    }
  }
  return best;
}

std::vector<jcad::app::Editor::Point2> jcad::app::Editor::RegularPolygonPoints(Point2 center,
                                                                               Point2 corner) const
{
  const auto arm{ corner - center };
  const double radius{ Length(arm) };
  if (radius <= 0.0)
    return {};
  const double rotation{ std::atan2(arm.y, arm.x) };
  const double step{ 2.0 * std::numbers::pi / m_polygon_sides };
  std::vector<Point2> points;
  for (int i{ 0 }; i < m_polygon_sides; ++i)
  {
    const double angle{ rotation + step * i };
    points.push_back(
      Point2{ center.x + radius * std::cos(angle), center.y + radius * std::sin(angle) });
  }
  return points;
}

std::optional<jcad::geom::Curve> jcad::app::Editor::ToolCurve(Point2 last) const
{
  if (m_pending.empty())
    return std::nullopt;
  const Point2 first{ m_pending.front() };
  const auto offset{ last - first };
  switch (m_tool)
  {
  case Tool::CIRCLE:
  {
    const double radius{ Length(offset) };
    if (radius <= 0.0)
      return std::nullopt;
    return geom::EllipticArc{ first, radius, radius, 0.0, 0.0, 2.0 * std::numbers::pi };
  }
  case Tool::ELLIPSE:
    if (offset.x == 0.0 || offset.y == 0.0)
      return std::nullopt;
    return geom::EllipticArc{ first, std::abs(offset.x),    std::abs(offset.y), 0.0,
                              0.0,   2.0 * std::numbers::pi };
  case Tool::ARC:
    if (m_pending.size() < 2)
      return std::nullopt;
    return geom::ArcThroughPoints(first, m_pending[1], last);
  case Tool::BEZIER:
  {
    std::vector<Point2> points{ m_pending };
    points.push_back(last);
    return geom::Bezier{ std::move(points) };
  }
  default:
    return std::nullopt;
  }
}

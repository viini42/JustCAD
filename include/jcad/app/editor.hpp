#pragma once

#include "app/tool.hpp"
#include "geom/curve.hpp"
#include "model/geometric_model.hpp"
#include "topo/id.hpp"

#include <optional>
#include <vector>

namespace jcad::app
{

  /**
   * The interaction logic of the drawing window, independent of any UI toolkit: it
   * turns clicks in world coordinates into model edits according to the current
   * tool, and keeps the selection and the shape being drawn.
   *
   * - POINT: one click. LINE and RECTANGLE: two clicks (ends, opposite corners).
   * - POLYLINE: one click per vertex; Finish() keeps it open, clicking its first
   *   vertex again closes it.
   * - POLYGON: center, then a vertex (sets radius and rotation).
   * - CIRCLE: center, then a point on it. ELLIPSE: center, then a corner of its box.
   * - ARC: start, a point it passes through, end.
   * - BEZIER: one click per control point (any degree); Finish() adds it.
   * - SELECT: picks the nearest vertex, else the nearest edge, within the pick radius.
   */
  class Editor
  {
  public:
    using Kernel = model::GeometricModel::Kernel;
    using Point2 = model::GeometricModel::Point2;

    struct Selection
    {
      topo::VertexId vertex{};
      topo::EdgeId edge{};
    };

    struct PreviewShape
    {
      std::vector<Point2> points;
      bool closed{ false };
    };

    Editor() = default;

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&) = delete;
    Editor& operator=(Editor&&) = delete;

    [[nodiscard]] const model::GeometricModel& Model() const noexcept;

    [[nodiscard]] Tool CurrentTool() const noexcept;
    /** Switching tools drops the shape being drawn. */
    void SetTool(Tool tool);

    [[nodiscard]] int PolygonSides() const noexcept;
    void SetPolygonSides(int sides);

    /** Grid spacing used for snapping; 0 disables the grid. */
    [[nodiscard]] double GridStep() const noexcept;
    void SetGridStep(double step);

    /** Radius, in world units, for picking and for snapping to vertices. */
    void SetPickRadius(double radius);

    /** The nearest vertex within the pick radius, else the nearest grid point. */
    [[nodiscard]] Point2 Snap(Point2 world) const;

    void Click(Point2 world);
    void Hover(Point2 world);
    void Finish();
    void Cancel();

    void DeleteSelection();
    void Undo();
    void Redo();

    [[nodiscard]] const Selection& GetSelection() const noexcept;
    [[nodiscard]] const std::vector<Point2>& PendingPoints() const noexcept;
    /** What the current tool would add if the user clicked at the hover point. */
    [[nodiscard]] PreviewShape Preview() const;

  private:
    void Select(Point2 world);
    void ClickPolyline(Point2 point);
    void Complete(Point2 point);
    [[nodiscard]] std::size_t ClicksNeeded() const noexcept;
    [[nodiscard]] std::optional<topo::VertexId> NearestVertex(Point2 world) const;
    [[nodiscard]] std::optional<topo::EdgeId> NearestEdge(Point2 world) const;
    [[nodiscard]] std::vector<Point2> RegularPolygonPoints(Point2 center, Point2 corner) const;
    /** The curve the current tool makes from the pending points plus `last`. */
    [[nodiscard]] std::optional<geom::Curve> ToolCurve(Point2 last) const;

    model::GeometricModel m_model;
    Tool m_tool{ Tool::SELECT };
    int m_polygon_sides{ 6 };
    double m_grid_step{ 0.0 };
    double m_pick_radius{ 0.1 };
    Selection m_selection;
    std::vector<Point2> m_pending;
    std::optional<Point2> m_hover;
  };

} // namespace jcad::app

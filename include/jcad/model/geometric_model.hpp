#pragma once

#include "euler/corner.hpp"
#include "euler/history.hpp"
#include "geom/double_kernel.hpp"
#include "topo/id.hpp"
#include "topo/topology.hpp"

#include <optional>
#include <vector>

namespace jcad::model
{

  /**
   * The public modeling API: primitives go in as geometry and are kept as a planar
   * arrangement in a half-edge topology.
   *
   * Every primitive is inserted edge by edge: its ends snap to existing vertices or
   * split the edges they lie on, existing edges it crosses are split, and each piece
   * is added with the corners its direction falls into. Closing a cycle creates a
   * face, so regions bounded by any mix of primitives appear on their own. Each
   * public modifying call is one undo entry.
   *
   * Only straight edges exist so far: an edge's geometry is the segment between its
   * vertices and its CurveId is left invalid.
   */
  class GeometricModel
  {
  public:
    using Kernel = geom::DoubleKernel;
    using Point2 = Kernel::Point2;

    struct RegularPolygon
    {
      Point2 center{};
      double radius{ 1.0 };
      int sides{ 3 };
      /** Angle of the first vertex, in radians, counter-clockwise from +x. */
      double rotation{ 0.0 };
    };

    struct EdgeSegment
    {
      topo::EdgeId edge{};
      Point2 start{};
      Point2 end{};
    };

    struct FaceOutline
    {
      topo::FaceId face{};
      /** Counter-clockwise. */
      std::vector<Point2> outer;
      /** Boundaries of whatever lies inside the face, clockwise. */
      std::vector<std::vector<Point2>> holes;
    };

    GeometricModel();

    GeometricModel(const GeometricModel&) = delete;
    GeometricModel& operator=(const GeometricModel&) = delete;
    GeometricModel(GeometricModel&&) = delete;
    GeometricModel& operator=(GeometricModel&&) = delete;

    [[nodiscard]] const topo::Topology& GetTopology() const noexcept;
    [[nodiscard]] Point2 PointOf(topo::VertexId vertex) const;

    topo::VertexId AddPoint(Point2 point);
    void AddLine(Point2 start, Point2 end);
    /** An open polyline, or a closed polygon when `closed` is true. */
    void AddPolyline(const std::vector<Point2>& points, bool closed);
    void AddRectangle(Point2 corner, Point2 opposite);
    void AddRegularPolygon(const RegularPolygon& polygon);

    /**
     * Removes an edge. Faces on its two sides merge; vertices left without edges are
     * removed, and a vertex left between two collinear edges is merged away.
     */
    void RemoveEdge(topo::EdgeId edge);
    /** Removes a vertex and its edges, with the same clean-up as RemoveEdge. */
    void RemoveVertex(topo::VertexId vertex);

    [[nodiscard]] bool CanUndo() const noexcept;
    [[nodiscard]] bool CanRedo() const noexcept;
    void Undo();
    void Redo();

    /** The innermost face containing `point`; the unbounded face if no other. */
    [[nodiscard]] topo::FaceId LocateFace(Point2 point) const;
    [[nodiscard]] std::vector<Point2> LoopPolygon(topo::LoopId loop) const;
    [[nodiscard]] double LoopSignedArea(topo::LoopId loop) const;
    [[nodiscard]] std::vector<EdgeSegment> EdgeSegments() const;
    [[nodiscard]] std::vector<FaceOutline> BoundedFaces() const;

  private:
    void AddSegment(Point2 start, Point2 end);
    topo::VertexId ResolvePoint(Point2 point);
    topo::VertexId SplitAt(topo::HalfEdgeId halfEdge, Point2 point);
    std::vector<topo::VertexId> VerticesAlong(topo::VertexId start, topo::VertexId end);
    void Connect(topo::VertexId from, topo::VertexId to);
    void AdoptEnclosedLoops(topo::FaceId newFace, topo::LoopId splitLoop);
    void RemoveEdgeKeepingOuterLoops(topo::HalfEdgeId halfEdge);
    void CleanUpVertex(topo::VertexId vertex);

    [[nodiscard]] std::optional<topo::VertexId> VertexAt(Point2 point) const;
    [[nodiscard]] std::optional<topo::HalfEdgeId> EdgeThrough(Point2 point) const;
    [[nodiscard]] bool Adjacent(topo::VertexId a, topo::VertexId b) const;
    [[nodiscard]] euler::Corner CornerToward(topo::VertexId vertex, Point2 target) const;
    [[nodiscard]] Point2 LoopSamplePoint(topo::LoopId loop) const;
    [[nodiscard]] topo::GeomPointId StorePoint(Point2 point);

    topo::Topology m_topology;
    euler::History m_history{ m_topology };
    std::vector<Point2> m_points;
  };

} // namespace jcad::model

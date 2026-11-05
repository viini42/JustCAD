#pragma once

#include "euler/corner.hpp"
#include "euler/history.hpp"
#include "geom/curve.hpp"
#include "geom/double_kernel.hpp"
#include "geom/elliptic_arc.hpp"
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
   * Every primitive is inserted curve by curve: its ends snap to existing vertices
   * or split the edges they lie on, existing edges it meets are split, and each
   * piece is added with the corners its direction falls into. Closing a cycle
   * creates a face, so regions bounded by any mix of primitives appear on their own.
   * Each public modifying call is one undo entry.
   *
   * Every edge has a curve (CurveId), oriented along the edge's first half-edge.
   * Closed curves (circles, ellipses) are stored as two edges. Areas and
   * containment are computed on polylines within TESSELLATION_TOLERANCE of the
   * curves.
   */
  class GeometricModel
  {
  public:
    using Kernel = geom::DoubleKernel;
    using Point2 = Kernel::Point2;

    static constexpr double TESSELLATION_TOLERANCE{ 1e-3 };

    struct RegularPolygon
    {
      Point2 center{};
      double radius{ 1.0 };
      int sides{ 3 };
      /** Angle of the first vertex, in radians, counter-clockwise from +x. */
      double rotation{ 0.0 };
    };

    struct EdgeCurve
    {
      topo::EdgeId edge{};
      geom::Curve curve;
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
    /** The edge's curve, running along its first half-edge. */
    [[nodiscard]] const geom::Curve& CurveOf(topo::EdgeId edge) const;
    /** The curve of the half-edge's edge, running along the half-edge. */
    [[nodiscard]] geom::Curve CurveAlong(topo::HalfEdgeId halfEdge) const;

    topo::VertexId AddPoint(Point2 point);
    void AddLine(Point2 start, Point2 end);
    /** An open polyline, or a closed polygon when `closed` is true. */
    void AddPolyline(const std::vector<Point2>& points, bool closed);
    void AddRectangle(Point2 corner, Point2 opposite);
    void AddRegularPolygon(const RegularPolygon& polygon);
    /** Any curve: segment, Bézier of any degree, or arc. A closed one makes a region. */
    void AddCurve(const geom::Curve& curve);
    void AddCircle(Point2 center, double radius);
    /** Full ellipse; only center, radii and rotation of `ellipse` are used. */
    void AddEllipse(const geom::EllipticArc& ellipse);

    /**
     * Removes an edge. Faces on its two sides merge; vertices left without edges are
     * removed, and a vertex left between two collinear segments is merged away.
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
    /** The loop as a polyline (curves tessellated). */
    [[nodiscard]] std::vector<Point2> LoopPolygon(topo::LoopId loop) const;
    [[nodiscard]] double LoopSignedArea(topo::LoopId loop) const;
    [[nodiscard]] std::vector<EdgeCurve> EdgeCurves() const;
    [[nodiscard]] std::vector<FaceOutline> BoundedFaces() const;

  private:
    struct Hit
    {
      double t{};
      topo::VertexId vertex{};
    };

    void InsertCurve(const geom::Curve& curve);
    topo::VertexId ResolvePoint(Point2 point);
    topo::VertexId SplitEdgeAt(topo::EdgeId edge, double t, Point2 point);
    std::vector<Hit> HitsAlong(const geom::Curve& curve, topo::VertexId start, topo::VertexId end);
    void SplitCrossedEdges(const geom::Curve& curve, std::vector<Hit>& hits);
    void InsertPiece(topo::VertexId from, topo::VertexId to, const geom::Curve& piece);
    void Connect(topo::VertexId from, topo::VertexId to, const geom::Curve& piece);
    void AdoptEnclosedLoops(topo::FaceId newFace, topo::LoopId splitLoop);
    void RemoveEdgeKeepingOuterLoops(topo::HalfEdgeId halfEdge);
    void CleanUpVertex(topo::VertexId vertex);

    [[nodiscard]] std::optional<topo::VertexId> VertexNear(Point2 point) const;
    [[nodiscard]] bool
    HasEdgeLike(topo::VertexId from, topo::VertexId to, const geom::Curve& piece) const;
    [[nodiscard]] euler::Corner CornerToward(topo::VertexId vertex, Point2 direction) const;
    [[nodiscard]] Point2 LoopSamplePoint(topo::LoopId loop) const;
    [[nodiscard]] topo::GeomPointId StorePoint(Point2 point);
    [[nodiscard]] topo::CurveId StoreCurve(geom::Curve curve);

    topo::Topology m_topology;
    euler::History m_history{ m_topology };
    std::vector<Point2> m_points;
    std::vector<geom::Curve> m_curves;
  };

} // namespace jcad::model

#include "model/geometric_model.hpp"

#include "core/assert.hpp"
#include "euler/euler_op.hpp"
#include "euler/high_level_ops.hpp"
#include "geom/polygon.hpp"
#include "geom/segment_crossing.hpp"
#include "geom/sign.hpp"
#include "topo/half_edge.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <utility>

namespace
{

  using Kernel = jcad::model::GeometricModel::Kernel;
  using Point2 = Kernel::Point2;
  using jcad::geom::Sign;
  using jcad::topo::EdgeId;
  using jcad::topo::FaceId;
  using jcad::topo::HalfEdgeId;
  using jcad::topo::LoopId;
  using jcad::topo::Topology;
  using jcad::topo::Twin;
  using jcad::topo::VertexId;

  /**
   * Whether direction center->d lies strictly inside the counter-clockwise sector
   * that goes from center->a to center->b. a == b means the full turn.
   */
  bool InCcwSector(Point2 center, Point2 a, Point2 b, Point2 d)
  {
    if (Kernel::Equal(a, b))
      return true;
    const Sign turn{ Kernel::Orientation(center, a, b) };
    if (turn == Sign::POSITIVE)
      return Kernel::Orientation(center, a, d) == Sign::POSITIVE &&
             Kernel::Orientation(center, d, b) == Sign::POSITIVE;
    if (turn == Sign::NEGATIVE)
      return !(Kernel::Orientation(center, b, d) != Sign::NEGATIVE &&
               Kernel::Orientation(center, d, a) != Sign::NEGATIVE);
    // a and b point in opposite directions: the sector is the half-plane left of a.
    return Kernel::Orientation(center, a, d) == Sign::POSITIVE;
  }

  /** Parameter of `p` along start->end, for sorting points on that segment. */
  double ParameterAlong(Point2 start, Point2 end, Point2 p)
  {
    const auto direction{ end - start };
    return Dot(p - start, direction) / SquaredLength(direction);
  }

  struct Hit
  {
    double t{};
    VertexId vertex{};
  };

} // namespace

jcad::model::GeometricModel::GeometricModel() = default;

const jcad::topo::Topology& jcad::model::GeometricModel::GetTopology() const noexcept
{
  return m_topology;
}

jcad::model::GeometricModel::Point2
jcad::model::GeometricModel::PointOf(topo::VertexId vertex) const
{
  return m_points[m_topology.VertexAt(vertex).point.Value()];
}

jcad::topo::VertexId jcad::model::GeometricModel::AddPoint(Point2 point)
{
  m_history.Begin();
  const VertexId vertex{ ResolvePoint(point) };
  m_history.Commit();
  return vertex;
}

void jcad::model::GeometricModel::AddLine(Point2 start, Point2 end)
{
  m_history.Begin();
  AddSegment(start, end);
  m_history.Commit();
}

void jcad::model::GeometricModel::AddPolyline(const std::vector<Point2>& points, bool closed)
{
  m_history.Begin();
  for (std::size_t i{ 1 }; i < points.size(); ++i)
    AddSegment(points[i - 1], points[i]);
  if (closed && points.size() > 2)
    AddSegment(points.back(), points.front());
  if (points.size() == 1)
    static_cast<void>(ResolvePoint(points.front()));
  m_history.Commit();
}

void jcad::model::GeometricModel::AddRectangle(Point2 corner, Point2 opposite)
{
  const std::vector<Point2> points{ corner,
                                    Point2{ opposite.x, corner.y },
                                    opposite,
                                    Point2{ corner.x, opposite.y } };
  AddPolyline(points, true);
}

void jcad::model::GeometricModel::AddRegularPolygon(const RegularPolygon& polygon)
{
  JCAD_ASSERT(polygon.sides >= 3, "a polygon needs at least 3 sides, got {}", polygon.sides);
  std::vector<Point2> points;
  const double step{ 2.0 * std::numbers::pi / polygon.sides };
  for (int i{ 0 }; i < polygon.sides; ++i)
  {
    const double angle{ polygon.rotation + step * i };
    points.push_back(Point2{ polygon.center.x + polygon.radius * std::cos(angle),
                             polygon.center.y + polygon.radius * std::sin(angle) });
  }
  AddPolyline(points, true);
}

void jcad::model::GeometricModel::RemoveEdge(topo::EdgeId edge)
{
  const HalfEdgeId half_edge{ topo::FirstHalfEdge(edge) };
  const VertexId start{ m_topology.HalfEdgeAt(half_edge).origin };
  const VertexId end{ m_topology.Target(half_edge) };
  m_history.Begin();
  RemoveEdgeKeepingOuterLoops(half_edge);
  CleanUpVertex(start);
  CleanUpVertex(end);
  m_history.Commit();
}

void jcad::model::GeometricModel::RemoveVertex(topo::VertexId vertex)
{
  m_history.Begin();
  std::vector<VertexId> neighbors;
  while (m_topology.VertexAt(vertex).outgoing.Valid())
  {
    const HalfEdgeId outgoing{ m_topology.VertexAt(vertex).outgoing };
    neighbors.push_back(m_topology.Target(outgoing));
    RemoveEdgeKeepingOuterLoops(outgoing);
  }
  m_history.Apply(euler::KillVertexRing{ vertex });
  for (const VertexId neighbor : neighbors)
    CleanUpVertex(neighbor);
  m_history.Commit();
}

bool jcad::model::GeometricModel::CanUndo() const noexcept
{
  return m_history.UndoCount() > 0;
}

bool jcad::model::GeometricModel::CanRedo() const noexcept
{
  return m_history.RedoCount() > 0;
}

void jcad::model::GeometricModel::Undo()
{
  m_history.Undo();
}

void jcad::model::GeometricModel::Redo()
{
  m_history.Redo();
}

jcad::topo::FaceId jcad::model::GeometricModel::LocateFace(Point2 point) const
{
  FaceId best{ m_topology.UnboundedFace() };
  double best_area{ std::numeric_limits<double>::infinity() };
  for (const FaceId face : m_topology.FaceIds())
  {
    const LoopId outer{ m_topology.FaceAt(face).outer };
    if (!outer.Valid())
      continue;
    const std::vector<Point2> polygon{ LoopPolygon(outer) };
    if (geom::LocatePoint<Kernel>(polygon, point) != geom::PointLocation::INSIDE)
      continue;
    // Faces containing the point are nested, so the innermost is the smallest.
    const double area{ geom::SignedArea(polygon) };
    if (area < best_area)
    {
      best = face;
      best_area = area;
    }
  }
  return best;
}

std::vector<jcad::model::GeometricModel::Point2>
jcad::model::GeometricModel::LoopPolygon(topo::LoopId loop) const
{
  std::vector<Point2> polygon;
  for (const HalfEdgeId half_edge : m_topology.LoopHalfEdges(loop))
    polygon.push_back(PointOf(m_topology.HalfEdgeAt(half_edge).origin));
  return polygon;
}

double jcad::model::GeometricModel::LoopSignedArea(topo::LoopId loop) const
{
  return geom::SignedArea(LoopPolygon(loop));
}

std::vector<jcad::model::GeometricModel::EdgeSegment>
jcad::model::GeometricModel::EdgeSegments() const
{
  std::vector<EdgeSegment> segments;
  for (const EdgeId edge : m_topology.EdgeIds())
  {
    const HalfEdgeId half_edge{ topo::FirstHalfEdge(edge) };
    segments.push_back(EdgeSegment{ edge,
                                    PointOf(m_topology.HalfEdgeAt(half_edge).origin),
                                    PointOf(m_topology.Target(half_edge)) });
  }
  return segments;
}

std::vector<jcad::model::GeometricModel::FaceOutline>
jcad::model::GeometricModel::BoundedFaces() const
{
  std::vector<FaceOutline> outlines;
  for (const FaceId face : m_topology.FaceIds())
  {
    const topo::Face& data{ m_topology.FaceAt(face) };
    if (!data.outer.Valid())
      continue;
    FaceOutline outline{ face, LoopPolygon(data.outer), {} };
    for (const LoopId inner : data.inner)
      outline.holes.push_back(LoopPolygon(inner));
    outlines.push_back(std::move(outline));
  }
  return outlines;
}

void jcad::model::GeometricModel::AddSegment(Point2 start, Point2 end)
{
  const VertexId first{ ResolvePoint(start) };
  const VertexId last{ ResolvePoint(end) };
  if (first == last)
    return;
  const std::vector<VertexId> chain{ VerticesAlong(first, last) };
  for (std::size_t i{ 1 }; i < chain.size(); ++i)
  {
    if (!Adjacent(chain[i - 1], chain[i]))
      Connect(chain[i - 1], chain[i]);
  }
}

jcad::topo::VertexId jcad::model::GeometricModel::ResolvePoint(Point2 point)
{
  if (const std::optional<VertexId> existing{ VertexAt(point) })
    return *existing;
  if (const std::optional<HalfEdgeId> edge{ EdgeThrough(point) })
    return SplitAt(*edge, point);
  return euler::AddIsolatedVertex(m_history, LocateFace(point), StorePoint(point));
}

jcad::topo::VertexId jcad::model::GeometricModel::SplitAt(topo::HalfEdgeId halfEdge, Point2 point)
{
  return euler::SplitEdge(m_history, halfEdge, StorePoint(point), euler::SplitCurves{});
}

std::vector<jcad::topo::VertexId> jcad::model::GeometricModel::VerticesAlong(topo::VertexId start,
                                                                             topo::VertexId end)
{
  const Point2 p{ PointOf(start) };
  const Point2 q{ PointOf(end) };
  std::vector<Hit> hits{ Hit{ 0.0, start }, Hit{ 1.0, end } };
  for (const VertexId vertex : m_topology.VertexIds())
  {
    const Point2 point{ PointOf(vertex) };
    if (vertex != start && vertex != end && Kernel::OnSegment(p, q, point))
      hits.push_back(Hit{ ParameterAlong(p, q, point), vertex });
  }
  // Collect first, split after: splitting changes the edge set being scanned.
  std::vector<std::pair<HalfEdgeId, Point2>> crossings;
  for (const EdgeSegment& segment : EdgeSegments())
  {
    const auto crossing{ geom::ProperCrossing<Kernel>(p, q, segment.start, segment.end) };
    if (crossing)
      crossings.emplace_back(topo::FirstHalfEdge(segment.edge), *crossing);
  }
  for (const auto& [half_edge, point] : crossings)
    hits.push_back(Hit{ ParameterAlong(p, q, point), SplitAt(half_edge, point) });

  std::ranges::sort(hits, {}, &Hit::t);
  std::vector<VertexId> chain;
  for (const Hit& hit : hits)
  {
    if (chain.empty() || chain.back() != hit.vertex)
      chain.push_back(hit.vertex);
  }
  return chain;
}

void jcad::model::GeometricModel::Connect(topo::VertexId from, topo::VertexId to)
{
  const euler::Corner from_corner{ CornerToward(from, PointOf(to)) };
  const euler::Corner to_corner{ CornerToward(to, PointOf(from)) };
  m_history.Begin();
  euler::AddedEdge added{ euler::AddEdge(m_history, from_corner, to_corner, topo::CurveId{}) };
  if (!added.new_face.Valid())
  {
    m_history.Commit();
    return;
  }
  // The new face must be the bounded, counter-clockwise side of the split loop.
  if (Kernel::SignOf(LoopSignedArea(m_topology.HalfEdgeAt(added.half_edge).loop)) != Sign::POSITIVE)
  {
    m_history.Rollback();
    m_history.Begin();
    added = euler::AddEdge(m_history, to_corner, from_corner, topo::CurveId{});
  }
  AdoptEnclosedLoops(added.new_face, m_topology.HalfEdgeAt(Twin(added.half_edge)).loop);
  m_history.Commit();
}

void jcad::model::GeometricModel::AdoptEnclosedLoops(topo::FaceId newFace, topo::LoopId splitLoop)
{
  const FaceId old_face{ m_topology.LoopAt(splitLoop).face };
  const std::vector<Point2> boundary{ LoopPolygon(m_topology.FaceAt(newFace).outer) };
  const std::vector<LoopId> inner{ m_topology.FaceAt(old_face).inner };
  for (const LoopId loop : inner)
  {
    if (loop == splitLoop)
      continue;
    // Separate components share no vertex with the boundary, so one sample decides.
    if (geom::LocatePoint<Kernel>(boundary, LoopSamplePoint(loop)) == geom::PointLocation::INSIDE)
      m_history.Apply(euler::MoveRing{ loop, newFace });
  }
}

void jcad::model::GeometricModel::RemoveEdgeKeepingOuterLoops(topo::HalfEdgeId halfEdge)
{
  const LoopId loop{ m_topology.HalfEdgeAt(halfEdge).loop };
  const bool bridge_of_outer{ loop == m_topology.HalfEdgeAt(Twin(halfEdge)).loop &&
                              m_topology.IsOuterLoop(loop) };
  m_history.Begin();
  euler::RemoveEdge(m_history, halfEdge);
  // Removing a bridge splits the outer loop; the part kept as outer must be the one
  // that still encloses the face, which only the geometry can tell.
  if (bridge_of_outer && Kernel::SignOf(LoopSignedArea(loop)) != Sign::POSITIVE)
  {
    m_history.Rollback();
    m_history.Begin();
    euler::RemoveEdge(m_history, Twin(halfEdge));
  }
  m_history.Commit();
}

void jcad::model::GeometricModel::CleanUpVertex(topo::VertexId vertex)
{
  if (!m_topology.Contains(vertex))
    return;
  const topo::Vertex& data{ m_topology.VertexAt(vertex) };
  if (data.ring.Valid())
  {
    if (!m_topology.IsOuterLoop(data.ring))
      m_history.Apply(euler::KillVertexRing{ vertex });
    return;
  }
  const std::vector<HalfEdgeId> outgoing{ m_topology.OutgoingHalfEdges(vertex) };
  if (outgoing.size() != 2)
    return;
  const VertexId a{ m_topology.Target(outgoing[0]) };
  const VertexId b{ m_topology.Target(outgoing[1]) };
  if (a != b && Kernel::OnSegment(PointOf(a), PointOf(b), PointOf(vertex)))
    euler::MergeEdges(m_history, Twin(outgoing[0]), topo::CurveId{});
}

std::optional<jcad::topo::VertexId> jcad::model::GeometricModel::VertexAt(Point2 point) const
{
  for (const VertexId vertex : m_topology.VertexIds())
  {
    if (Kernel::Equal(PointOf(vertex), point))
      return vertex;
  }
  return std::nullopt;
}

std::optional<jcad::topo::HalfEdgeId> jcad::model::GeometricModel::EdgeThrough(Point2 point) const
{
  for (const EdgeSegment& segment : EdgeSegments())
  {
    if (Kernel::OnSegment(segment.start, segment.end, point))
      return topo::FirstHalfEdge(segment.edge);
  }
  return std::nullopt;
}

bool jcad::model::GeometricModel::Adjacent(topo::VertexId a, topo::VertexId b) const
{
  const std::vector<HalfEdgeId> outgoing{ m_topology.OutgoingHalfEdges(a) };
  return std::ranges::any_of(outgoing, [&](HalfEdgeId h) { return m_topology.Target(h) == b; });
}

jcad::euler::Corner jcad::model::GeometricModel::CornerToward(topo::VertexId vertex,
                                                              Point2 target) const
{
  const std::vector<HalfEdgeId> outgoing{ m_topology.OutgoingHalfEdges(vertex) };
  if (outgoing.empty())
    return euler::Corner{ vertex, HalfEdgeId{} };
  const Point2 center{ PointOf(vertex) };
  // OutgoingHalfEdges() turns clockwise, so the counter-clockwise neighbour of
  // outgoing[i] is outgoing[i - 1]; the corner before outgoing[i] is that sector.
  const std::size_t count{ outgoing.size() };
  for (std::size_t i{ 0 }; i < count; ++i)
  {
    const HalfEdgeId half_edge{ outgoing[i] };
    const HalfEdgeId neighbor{ outgoing[(i + count - 1) % count] };
    const Point2 a{ PointOf(m_topology.Target(half_edge)) };
    const Point2 b{ PointOf(m_topology.Target(neighbor)) };
    if (InCcwSector(center, a, b, target))
      return euler::Corner{ vertex, half_edge };
  }
  JCAD_ASSERT(false, "no sector around vertex {} contains the direction", vertex.Value());
  return euler::Corner{ vertex, outgoing.front() };
}

jcad::model::GeometricModel::Point2
jcad::model::GeometricModel::LoopSamplePoint(topo::LoopId loop) const
{
  const topo::Loop& data{ m_topology.LoopAt(loop) };
  if (data.isolated_vertex.Valid())
    return PointOf(data.isolated_vertex);
  return PointOf(m_topology.HalfEdgeAt(data.first).origin);
}

jcad::topo::GeomPointId jcad::model::GeometricModel::StorePoint(Point2 point)
{
  m_points.push_back(point);
  return topo::GeomPointId{ static_cast<topo::GeomPointId::ValueType>(m_points.size() - 1) };
}

#include "model/geometric_model.hpp"

#include "core/assert.hpp"
#include "euler/euler_op.hpp"
#include "euler/high_level_ops.hpp"
#include "geom/curve_intersection.hpp"
#include "geom/line_segment.hpp"
#include "geom/polygon.hpp"
#include "geom/sign.hpp"
#include "topo/half_edge.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <numbers>
#include <utility>

namespace
{

  using Kernel = jcad::model::GeometricModel::Kernel;
  using Point2d = Kernel::Point2;
  using jcad::geom::Curve;
  using jcad::geom::LineSegment;
  using jcad::geom::Sign;
  using jcad::topo::EdgeId;
  using jcad::topo::FaceId;
  using jcad::topo::HalfEdgeId;
  using jcad::topo::LoopId;
  using jcad::topo::Topology;
  using jcad::topo::Twin;
  using jcad::topo::VertexId;

  /** How far along a curve, in world units, DirectionPoint() reads its direction. */
  constexpr double DIRECTION_DISTANCE{ 1e-3 };
  /** Two edges between the same vertices are the same edge when their midpoints are this close. */
  constexpr double SAME_EDGE_DISTANCE{ 1e-6 };
  /**
   * A crossing this close to a vertex lands on it. Tangential contacts are only
   * located to about the square root of the kernel tolerance along the curves.
   */
  constexpr double CONTACT_SNAP_DISTANCE{ 1e-6 };

  /**
   * Whether direction center->d lies strictly inside the counter-clockwise sector
   * that goes from center->a to center->b. a == b means the full turn.
   */
  bool InCcwSector(Point2d center, Point2d a, Point2d b, Point2d d)
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

  /** Rough length of a curve, from its polyline. */
  double ApproximateLength(const Curve& curve)
  {
    const std::vector<Point2d> points{ jcad::geom::Tessellate(curve, DIRECTION_DISTANCE) };
    double length{ 0.0 };
    for (std::size_t i{ 1 }; i < points.size(); ++i)
      length += Length(points[i] - points[i - 1]);
    return length;
  }

  /**
   * A point at unit distance from the curve's start, in the direction the curve
   * leaves it. Reading the curve a fixed distance in (rather than its tangent, or
   * a fixed fraction of a possibly tiny piece) tells apart curves that leave a
   * vertex tangent to each other.
   */
  Point2d DirectionPoint(const Curve& curve)
  {
    const Point2d start{ jcad::geom::StartPoint(curve) };
    Point2d ahead{ jcad::geom::EndPoint(curve) };
    if (!std::holds_alternative<LineSegment>(curve))
    {
      const double length{ ApproximateLength(curve) };
      const double t{ length > 0.0 ? std::min(0.5, DIRECTION_DISTANCE / length) : 0.5 };
      ahead = jcad::geom::Eval(curve, t);
    }
    const auto offset{ ahead - start };
    return start + offset / Length(offset);
  }

  /** Makes the piece start and end exactly at its vertices where the curve kind allows. */
  Curve WithEnds(Curve curve, Point2d start, Point2d end)
  {
    if (auto* line{ std::get_if<LineSegment>(&curve) })
      *line = LineSegment{ start, end };
    if (auto* bezier{ std::get_if<jcad::geom::Bezier>(&curve) })
    {
      bezier->control_points.front() = start;
      bezier->control_points.back() = end;
    }
    return curve;
  }

  struct Crossing
  {
    double t_curve{};
    double t_edge{};
    Point2d point{};
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

const jcad::geom::Curve& jcad::model::GeometricModel::CurveOf(topo::EdgeId edge) const
{
  return m_curves[m_topology.EdgeAt(edge).curve.Value()];
}

jcad::geom::Curve jcad::model::GeometricModel::CurveAlong(topo::HalfEdgeId halfEdge) const
{
  const Curve& curve{ CurveOf(topo::EdgeOf(halfEdge)) };
  return halfEdge == topo::FirstHalfEdge(topo::EdgeOf(halfEdge)) ? curve : geom::Reversed(curve);
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
  AddCurve(LineSegment{ start, end });
}

void jcad::model::GeometricModel::AddPolyline(const std::vector<Point2>& points, bool closed)
{
  m_history.Begin();
  for (std::size_t i{ 1 }; i < points.size(); ++i)
    InsertCurve(LineSegment{ points[i - 1], points[i] });
  if (closed && points.size() > 2)
    InsertCurve(LineSegment{ points.back(), points.front() });
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

void jcad::model::GeometricModel::AddCurve(const geom::Curve& curve)
{
  m_history.Begin();
  InsertCurve(curve);
  m_history.Commit();
}

void jcad::model::GeometricModel::AddCircle(Point2 center, double radius)
{
  AddEllipse(geom::EllipticArc{ center, radius, radius, 0.0, 0.0, 0.0 });
}

void jcad::model::GeometricModel::AddEllipse(const geom::EllipticArc& ellipse)
{
  geom::EllipticArc full{ ellipse };
  full.start_angle = 0.0;
  full.sweep = 2.0 * std::numbers::pi;
  AddCurve(full);
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
  {
    std::vector<Point2> points{ geom::Tessellate(CurveAlong(half_edge), TESSELLATION_TOLERANCE) };
    // The last point is the start of the next half-edge.
    polygon.insert(polygon.end(), points.begin(), points.end() - 1);
  }
  return polygon;
}

double jcad::model::GeometricModel::LoopSignedArea(topo::LoopId loop) const
{
  return geom::SignedArea(LoopPolygon(loop));
}

std::vector<jcad::model::GeometricModel::EdgeCurve> jcad::model::GeometricModel::EdgeCurves() const
{
  std::vector<EdgeCurve> curves;
  for (const EdgeId edge : m_topology.EdgeIds())
    curves.push_back(EdgeCurve{ edge, CurveOf(edge) });
  return curves;
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

void jcad::model::GeometricModel::InsertCurve(const geom::Curve& curve)
{
  const Point2 start{ geom::StartPoint(curve) };
  const Point2 end{ geom::EndPoint(curve) };
  if (Kernel::Equal(start, end))
  {
    // A closed curve would be a self-loop edge: insert it as two halves.
    if (Kernel::Equal(geom::Eval(curve, 0.5), start))
      return;
    const auto [first, second]{ geom::Split(curve, 0.5) };
    InsertCurve(first);
    InsertCurve(second);
    return;
  }
  const VertexId first{ ResolvePoint(start) };
  const VertexId last{ ResolvePoint(end) };
  if (first == last)
    return;
  std::vector<Hit> hits{ HitsAlong(curve, first, last) };
  for (std::size_t i{ 1 }; i < hits.size(); ++i)
  {
    if (hits[i].t > hits[i - 1].t)
      InsertPiece(hits[i - 1].vertex,
                  hits[i].vertex,
                  geom::SubCurve(curve, hits[i - 1].t, hits[i].t));
  }
}

jcad::topo::VertexId jcad::model::GeometricModel::ResolvePoint(Point2 point)
{
  if (const std::optional<VertexId> existing{ VertexAt(point) })
    return *existing;
  for (const EdgeId edge : m_topology.EdgeIds())
  {
    const double t{ geom::ClosestParam(CurveOf(edge), point) };
    if (t > 0.0 && t < 1.0 && Kernel::Equal(geom::Eval(CurveOf(edge), t), point))
      return SplitEdgeAt(edge, t, point);
  }
  return euler::AddIsolatedVertex(m_history, LocateFace(point), StorePoint(point));
}

jcad::topo::VertexId
jcad::model::GeometricModel::SplitEdgeAt(topo::EdgeId edge, double t, Point2 point)
{
  const HalfEdgeId half_edge{ topo::FirstHalfEdge(edge) };
  const Point2 start{ PointOf(m_topology.HalfEdgeAt(half_edge).origin) };
  const Point2 end{ PointOf(m_topology.Target(half_edge)) };
  const auto [before, after]{ geom::Split(CurveOf(edge), t) };
  const euler::SplitCurves curves{ StoreCurve(WithEnds(before, start, point)),
                                   StoreCurve(WithEnds(after, point, end)) };
  return euler::SplitEdge(m_history, half_edge, StorePoint(point), curves);
}

std::vector<jcad::model::GeometricModel::Hit>
jcad::model::GeometricModel::HitsAlong(const geom::Curve& curve,
                                       topo::VertexId start,
                                       topo::VertexId end)
{
  std::vector<Hit> hits{ Hit{ 0.0, start }, Hit{ 1.0, end } };
  for (const VertexId vertex : m_topology.VertexIds())
  {
    if (vertex == start || vertex == end)
      continue;
    const Point2 point{ PointOf(vertex) };
    const double t{ geom::ClosestParam(curve, point) };
    if (Kernel::Equal(geom::Eval(curve, t), point))
      hits.push_back(Hit{ t, vertex });
  }
  SplitCrossedEdges(curve, hits);
  std::ranges::sort(hits, {}, &Hit::t);
  const auto duplicates{ std::ranges::unique(hits, {}, &Hit::vertex) };
  hits.erase(duplicates.begin(), duplicates.end());
  return hits;
}

void jcad::model::GeometricModel::SplitCrossedEdges(const geom::Curve& curve,
                                                    std::vector<Hit>& hits)
{
  // Collect first, split after: splitting changes the edges being scanned. One edge
  // can be crossed several times; splitting it from its far end keeps the
  // parameters of the remaining crossings valid on the part that keeps its Id.
  std::map<EdgeId, std::vector<Crossing>> crossings;
  for (const EdgeId edge : m_topology.EdgeIds())
  {
    for (const geom::CurveIntersection& hit :
         geom::Intersect(curve, CurveOf(edge), TESSELLATION_TOLERANCE))
      crossings[edge].push_back(Crossing{ hit.t_a, hit.t_b, hit.point });
  }
  for (auto& [edge, list] : crossings)
  {
    std::ranges::sort(list, std::greater{}, &Crossing::t_edge);
    double kept{ 1.0 };
    for (const Crossing& crossing : list)
    {
      const std::optional<VertexId> existing{ VertexNear(crossing.point) };
      const double t{ crossing.t_edge / kept };
      const VertexId vertex{ existing ? *existing : SplitEdgeAt(edge, t, crossing.point) };
      if (!existing)
        kept = crossing.t_edge;
      hits.push_back(Hit{ crossing.t_curve, vertex });
    }
  }
}

void jcad::model::GeometricModel::InsertPiece(topo::VertexId from,
                                              topo::VertexId to,
                                              const geom::Curve& piece)
{
  if (from == to)
  {
    // The piece closes on itself: give it a middle vertex so no edge is a self-loop.
    const auto [first, second]{ geom::Split(piece, 0.5) };
    const VertexId middle{ ResolvePoint(geom::EndPoint(first)) };
    if (middle == from)
      return;
    InsertPiece(from, middle, first);
    InsertPiece(middle, to, second);
    return;
  }
  const Curve exact{ WithEnds(piece, PointOf(from), PointOf(to)) };
  if (!HasEdgeLike(from, to, exact))
    Connect(from, to, exact);
}

void jcad::model::GeometricModel::Connect(topo::VertexId from,
                                          topo::VertexId to,
                                          const geom::Curve& piece)
{
  const euler::Corner from_corner{ CornerToward(from, DirectionPoint(piece)) };
  const euler::Corner to_corner{ CornerToward(to, DirectionPoint(geom::Reversed(piece))) };
  const topo::CurveId forward{ StoreCurve(piece) };
  m_history.Begin();
  euler::AddedEdge added{ euler::AddEdge(m_history, from_corner, to_corner, forward) };
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
    added = euler::AddEdge(m_history, to_corner, from_corner, StoreCurve(geom::Reversed(piece)));
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
  const bool both_lines{ std::ranges::all_of(
    outgoing,
    [&](HalfEdgeId h) { return std::holds_alternative<LineSegment>(CurveOf(topo::EdgeOf(h))); }) };
  const VertexId a{ m_topology.Target(outgoing[0]) };
  const VertexId b{ m_topology.Target(outgoing[1]) };
  if (!both_lines || a == b || !Kernel::OnSegment(PointOf(a), PointOf(b), PointOf(vertex)))
    return;
  // The joined edge keeps the edge of `into` (a -> vertex); its curve runs along
  // that edge's first half-edge.
  const HalfEdgeId into{ Twin(outgoing[0]) };
  const bool into_is_first{ into == topo::FirstHalfEdge(topo::EdgeOf(into)) };
  const LineSegment joined{ into_is_first ? LineSegment{ PointOf(a), PointOf(b) }
                                          : LineSegment{ PointOf(b), PointOf(a) } };
  euler::MergeEdges(m_history, into, StoreCurve(joined));
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

std::optional<jcad::topo::VertexId> jcad::model::GeometricModel::VertexNear(Point2 point) const
{
  std::optional<VertexId> nearest;
  double nearest_distance{ CONTACT_SNAP_DISTANCE };
  for (const VertexId vertex : m_topology.VertexIds())
  {
    const double distance{ Length(PointOf(vertex) - point) };
    if (distance <= nearest_distance)
    {
      nearest = vertex;
      nearest_distance = distance;
    }
  }
  return nearest;
}

bool jcad::model::GeometricModel::HasEdgeLike(topo::VertexId from,
                                              topo::VertexId to,
                                              const geom::Curve& piece) const
{
  const Point2 middle{ geom::Eval(piece, 0.5) };
  const std::vector<HalfEdgeId> outgoing{ m_topology.OutgoingHalfEdges(from) };
  return std::ranges::any_of(outgoing,
                             [&](HalfEdgeId h)
                             {
                               return m_topology.Target(h) == to &&
                                      Length(geom::Eval(CurveAlong(h), 0.5) - middle) <=
                                        SAME_EDGE_DISTANCE;
                             });
}

jcad::euler::Corner jcad::model::GeometricModel::CornerToward(topo::VertexId vertex,
                                                              Point2 direction) const
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
    const Point2 a{ DirectionPoint(CurveAlong(half_edge)) };
    const Point2 b{ DirectionPoint(CurveAlong(neighbor)) };
    if (InCcwSector(center, a, b, direction))
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

jcad::topo::CurveId jcad::model::GeometricModel::StoreCurve(geom::Curve curve)
{
  m_curves.push_back(std::move(curve));
  return topo::CurveId{ static_cast<topo::CurveId::ValueType>(m_curves.size() - 1) };
}

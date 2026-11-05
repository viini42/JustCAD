#include "euler/euler_op.hpp"

#include "core/assert.hpp"
#include "topo/half_edge.hpp"
#include "topo/topology.hpp"

#include <utility>
#include <variant>

namespace
{

  using jcad::euler::Corner;
  using jcad::topo::CurveId;
  using jcad::topo::Edge;
  using jcad::topo::EdgeOf;
  using jcad::topo::Face;
  using jcad::topo::FaceId;
  using jcad::topo::FirstHalfEdge;
  using jcad::topo::HalfEdgeId;
  using jcad::topo::Loop;
  using jcad::topo::LoopId;
  using jcad::topo::Topology;
  using jcad::topo::Twin;
  using jcad::topo::Vertex;
  using jcad::topo::VertexId;

  void Link(Topology& topology, HalfEdgeId from, HalfEdgeId to)
  {
    topology.HalfEdgeAt(from).next = to;
    topology.HalfEdgeAt(to).prev = from;
  }

  HalfEdgeId Next(const Topology& topology, HalfEdgeId halfEdge)
  {
    return topology.HalfEdgeAt(halfEdge).next;
  }

  HalfEdgeId Prev(const Topology& topology, HalfEdgeId halfEdge)
  {
    return topology.HalfEdgeAt(halfEdge).prev;
  }

  /** Creates an edge and returns the half-edge that will run from -> to. */
  HalfEdgeId NewEdge(Topology& topology, HalfEdgeId requested, CurveId curve)
  {
    if (!requested.Valid())
      return FirstHalfEdge(topology.AddEdge(Edge{ curve }));
    topology.AddEdgeAt(EdgeOf(requested), Edge{ curve });
    return requested;
  }

  VertexId NewVertex(Topology& topology, VertexId requested, Vertex vertex)
  {
    if (!requested.Valid())
      return topology.AddVertex(vertex);
    topology.AddVertexAt(requested, vertex);
    return requested;
  }

  LoopId NewLoop(Topology& topology, LoopId requested, Loop loop)
  {
    if (!requested.Valid())
      return topology.AddLoop(loop);
    topology.AddLoopAt(requested, loop);
    return requested;
  }

  FaceId NewFace(Topology& topology, FaceId requested, Face face)
  {
    if (!requested.Valid())
      return topology.AddFace(std::move(face));
    topology.AddFaceAt(requested, std::move(face));
    return requested;
  }

  void SetEnds(Topology& topology, HalfEdgeId halfEdge, VertexId from, VertexId to)
  {
    topology.HalfEdgeAt(halfEdge).origin = from;
    topology.HalfEdgeAt(Twin(halfEdge)).origin = to;
  }

  /** Sets the loop of every half-edge in the cycle that starts at `start`. */
  void AssignLoop(Topology& topology, HalfEdgeId start, LoopId loop)
  {
    HalfEdgeId current{ start };
    do
    {
      topology.HalfEdgeAt(current).loop = loop;
      current = Next(topology, current);
    } while (current != start);
  }

  /** Turns `loop` into the ring of `vertex`, which has just lost its last edge. */
  void MakeRingOf(Topology& topology, VertexId vertex, LoopId loop)
  {
    Loop& data{ topology.LoopAt(loop) };
    data.first = HalfEdgeId{};
    data.isolated_vertex = vertex;
    Vertex& vertex_data{ topology.VertexAt(vertex) };
    vertex_data.ring = loop;
    vertex_data.outgoing = HalfEdgeId{};
  }

  /** Gives a vertex that was isolated its first outgoing half-edge. */
  void AttachFirstEdge(Topology& topology, VertexId vertex, HalfEdgeId outgoing)
  {
    Vertex& data{ topology.VertexAt(vertex) };
    if (data.outgoing.Valid())
      return;
    data.ring = LoopId{};
    data.outgoing = outgoing;
  }

  void
  ReplaceOutgoing(Topology& topology, VertexId vertex, HalfEdgeId removed, HalfEdgeId replacement)
  {
    Vertex& data{ topology.VertexAt(vertex) };
    if (data.outgoing == removed)
      data.outgoing = replacement;
  }

  void ReplaceFirst(Topology& topology, LoopId loop, HalfEdgeId halfEdge, HalfEdgeId replacement)
  {
    Loop& data{ topology.LoopAt(loop) };
    if (data.first == halfEdge || data.first == Twin(halfEdge))
      data.first = replacement;
  }

  void CheckCorner([[maybe_unused]] const Topology& topology, [[maybe_unused]] const Corner& corner)
  {
    JCAD_ASSERT(topology.Contains(corner.vertex),
                "corner vertex {} is dead",
                corner.vertex.Value());
    if (corner.outgoing.Valid())
    {
      JCAD_ASSERT(topology.HalfEdgeAt(corner.outgoing).origin == corner.vertex,
                  "half-edge {} does not leave vertex {}",
                  corner.outgoing.Value(),
                  corner.vertex.Value());
    }
    else
    {
      JCAD_ASSERT(topology.VertexAt(corner.vertex).ring.Valid(),
                  "vertex {} has edges, the corner must name one",
                  corner.vertex.Value());
    }
  }

} // namespace

jcad::topo::LoopId jcad::euler::LoopOf(const topo::Topology& topology, const Corner& corner)
{
  CheckCorner(topology, corner);
  if (corner.outgoing.Valid())
    return topology.HalfEdgeAt(corner.outgoing).loop;
  return topology.VertexAt(corner.vertex).ring;
}

jcad::euler::EulerOp jcad::euler::Apply(topo::Topology& topology, const EulerOp& op)
{
  return std::visit([&](const auto& concrete) -> EulerOp { return Apply(topology, concrete); }, op);
}

jcad::euler::KillVertexRing jcad::euler::Apply(topo::Topology& topology, const MakeVertexRing& op)
{
  JCAD_ASSERT(topology.Contains(op.face), "face {} is dead", op.face.Value());
  const VertexId vertex{ NewVertex(topology, op.new_vertex, Vertex{ {}, {}, op.point }) };
  const LoopId loop{ NewLoop(topology, op.new_loop, Loop{ {}, vertex, op.face }) };
  topology.VertexAt(vertex).ring = loop;
  topology.AttachInnerLoop(op.face, loop);
  return KillVertexRing{ vertex };
}

jcad::euler::MakeVertexRing jcad::euler::Apply(topo::Topology& topology, const KillVertexRing& op)
{
  const Vertex vertex{ topology.VertexAt(op.vertex) };
  JCAD_ASSERT(vertex.ring.Valid(), "vertex {} still has edges", op.vertex.Value());
  JCAD_ASSERT(!topology.IsOuterLoop(vertex.ring),
              "ring of vertex {} bounds a face",
              op.vertex.Value());
  const FaceId face{ topology.LoopAt(vertex.ring).face };
  topology.DetachInnerLoop(face, vertex.ring);
  topology.RemoveLoop(vertex.ring);
  topology.RemoveVertex(op.vertex);
  return MakeVertexRing{ face, vertex.point, op.vertex, vertex.ring };
}

jcad::euler::KillEdgeVertex jcad::euler::Apply(topo::Topology& topology, const MakeEdgeVertex& op)
{
  const LoopId loop{ LoopOf(topology, op.from) };
  const VertexId from{ op.from.vertex };
  const VertexId to{ NewVertex(topology, op.new_vertex, Vertex{ {}, {}, op.point }) };
  const HalfEdgeId out{ NewEdge(topology, op.new_half_edge, op.curve) };
  const HalfEdgeId back{ Twin(out) };
  SetEnds(topology, out, from, to);
  topology.HalfEdgeAt(out).loop = loop;
  topology.HalfEdgeAt(back).loop = loop;
  if (op.from.outgoing.Valid())
  {
    // The dangling edge goes out and comes back in the corner before `outgoing`.
    Link(topology, Prev(topology, op.from.outgoing), out);
    Link(topology, back, op.from.outgoing);
  }
  else
  {
    Link(topology, back, out);
    Loop& loop_data{ topology.LoopAt(loop) };
    loop_data.isolated_vertex = VertexId{};
    loop_data.first = out;
  }
  Link(topology, out, back);
  AttachFirstEdge(topology, from, out);
  topology.VertexAt(to).outgoing = back;
  return KillEdgeVertex{ out };
}

jcad::euler::MakeEdgeVertex jcad::euler::Apply(topo::Topology& topology, const KillEdgeVertex& op)
{
  const HalfEdgeId out{ op.half_edge };
  const HalfEdgeId back{ Twin(out) };
  JCAD_ASSERT(Next(topology, out) == back, "target of half-edge {} has other edges", out.Value());
  const VertexId from{ topology.HalfEdgeAt(out).origin };
  const VertexId to{ topology.HalfEdgeAt(back).origin };
  const LoopId loop{ topology.HalfEdgeAt(out).loop };
  const Vertex killed{ topology.VertexAt(to) };
  const CurveId curve{ topology.EdgeAt(EdgeOf(out)).curve };
  Corner corner{ from, HalfEdgeId{} };
  if (Next(topology, back) == out)
  {
    MakeRingOf(topology, from, loop);
  }
  else
  {
    corner.outgoing = Next(topology, back);
    Link(topology, Prev(topology, out), corner.outgoing);
    ReplaceFirst(topology, loop, out, corner.outgoing);
    ReplaceOutgoing(topology, from, out, corner.outgoing);
  }
  topology.RemoveEdge(EdgeOf(out));
  topology.RemoveVertex(to);
  return MakeEdgeVertex{ corner, killed.point, curve, to, out };
}

jcad::euler::KillEdgeFace jcad::euler::Apply(topo::Topology& topology, const MakeEdgeFace& op)
{
  const LoopId loop{ LoopOf(topology, op.from) };
  JCAD_ASSERT(LoopOf(topology, op.to) == loop, "MEF needs both corners in the same loop");
  JCAD_ASSERT(op.from.vertex != op.to.vertex, "MEF cannot make a self-loop edge");
  // Two different vertices in one loop both have edges, so both corners are real.
  const HalfEdgeId prev_from{ Prev(topology, op.from.outgoing) };
  const HalfEdgeId prev_to{ Prev(topology, op.to.outgoing) };

  const HalfEdgeId out{ NewEdge(topology, op.new_half_edge, op.curve) };
  const HalfEdgeId back{ Twin(out) };
  SetEnds(topology, out, op.from.vertex, op.to.vertex);
  const FaceId face{ NewFace(topology, op.new_face, Face{}) };
  const LoopId new_loop{ NewLoop(topology, op.new_loop, Loop{ out, {}, face }) };
  topology.FaceAt(face).outer = new_loop;

  Link(topology, prev_from, out);
  Link(topology, out, op.to.outgoing);
  Link(topology, prev_to, back);
  Link(topology, back, op.from.outgoing);
  AssignLoop(topology, out, new_loop);
  topology.HalfEdgeAt(back).loop = loop;
  topology.LoopAt(loop).first = back;
  return KillEdgeFace{ out };
}

jcad::euler::MakeEdgeFace jcad::euler::Apply(topo::Topology& topology, const KillEdgeFace& op)
{
  const HalfEdgeId out{ op.half_edge };
  const HalfEdgeId back{ Twin(out) };
  const LoopId killed_loop{ topology.HalfEdgeAt(out).loop };
  const LoopId kept_loop{ topology.HalfEdgeAt(back).loop };
  const FaceId killed_face{ topology.LoopAt(killed_loop).face };
  JCAD_ASSERT(killed_loop != kept_loop, "KEF needs the edge between two loops; use KEMR");
  JCAD_ASSERT(topology.IsOuterLoop(killed_loop), "KEF kills a face through its outer loop");
  JCAD_ASSERT(topology.FaceAt(killed_face).inner.empty(),
              "face {} still has inner loops",
              killed_face.Value());

  const VertexId from{ topology.HalfEdgeAt(out).origin };
  const VertexId to{ topology.HalfEdgeAt(back).origin };
  const HalfEdgeId next_out{ Next(topology, out) };
  const HalfEdgeId next_back{ Next(topology, back) };
  const CurveId curve{ topology.EdgeAt(EdgeOf(out)).curve };

  Link(topology, Prev(topology, out), next_back);
  Link(topology, Prev(topology, back), next_out);
  AssignLoop(topology, next_out, kept_loop);
  ReplaceFirst(topology, kept_loop, back, next_back);
  ReplaceOutgoing(topology, from, out, next_back);
  ReplaceOutgoing(topology, to, back, next_out);

  topology.RemoveEdge(EdgeOf(out));
  topology.RemoveLoop(killed_loop);
  topology.RemoveFace(killed_face);
  return MakeEdgeFace{
    Corner{ from, next_back }, Corner{ to, next_out }, curve, out, killed_face, killed_loop
  };
}

jcad::euler::MakeEdgeKillRing jcad::euler::Apply(topo::Topology& topology,
                                                 const KillEdgeMakeRing& op)
{
  const HalfEdgeId out{ op.half_edge };
  const HalfEdgeId back{ Twin(out) };
  const LoopId loop{ topology.HalfEdgeAt(out).loop };
  JCAD_ASSERT(topology.HalfEdgeAt(back).loop == loop, "KEMR needs both half-edges in one loop");
  const FaceId face{ topology.LoopAt(loop).face };
  const VertexId from{ topology.HalfEdgeAt(out).origin };
  const VertexId to{ topology.HalfEdgeAt(back).origin };
  const CurveId curve{ topology.EdgeAt(EdgeOf(out)).curve };
  // Removing the edge cuts the cycle into the part after `out` (around `to`) and
  // the part after `back` (around `from`); either part may be empty.
  const HalfEdgeId next_out{ Next(topology, out) };
  const HalfEdgeId next_back{ Next(topology, back) };
  const HalfEdgeId prev_out{ Prev(topology, out) };
  const HalfEdgeId prev_back{ Prev(topology, back) };
  const bool from_side_empty{ next_back == out };
  const bool to_side_empty{ next_out == back };

  const LoopId ring{ NewLoop(topology, op.new_loop, Loop{ {}, {}, face }) };
  topology.AttachInnerLoop(face, ring);
  Corner from_corner{ from, HalfEdgeId{} };
  Corner to_corner{ to, HalfEdgeId{} };
  if (from_side_empty)
  {
    MakeRingOf(topology, from, loop);
  }
  else
  {
    from_corner.outgoing = next_back;
    Link(topology, prev_out, next_back);
    topology.LoopAt(loop).first = next_back;
    ReplaceOutgoing(topology, from, out, next_back);
  }
  if (to_side_empty)
  {
    MakeRingOf(topology, to, ring);
  }
  else
  {
    to_corner.outgoing = next_out;
    Link(topology, prev_back, next_out);
    topology.LoopAt(ring).first = next_out;
    AssignLoop(topology, next_out, ring);
    ReplaceOutgoing(topology, to, back, next_out);
  }
  topology.RemoveEdge(EdgeOf(out));
  return MakeEdgeKillRing{ from_corner, to_corner, curve, out };
}

jcad::euler::KillEdgeMakeRing jcad::euler::Apply(topo::Topology& topology,
                                                 const MakeEdgeKillRing& op)
{
  const LoopId from_loop{ LoopOf(topology, op.from) };
  const LoopId to_loop{ LoopOf(topology, op.to) };
  const FaceId face{ topology.LoopAt(from_loop).face };
  JCAD_ASSERT(from_loop != to_loop, "MEKR needs two different loops; use MEF");
  JCAD_ASSERT(op.from.vertex != op.to.vertex, "MEKR cannot make a self-loop edge");
  JCAD_ASSERT(topology.LoopAt(to_loop).face == face, "MEKR needs both loops in the same face");
  const bool keep_to{ topology.IsOuterLoop(to_loop) };
  const LoopId loop{ keep_to ? to_loop : from_loop };
  const LoopId ring{ keep_to ? from_loop : to_loop };
  // Read the corners' predecessors before relinking anything.
  const bool from_isolated{ !op.from.outgoing.Valid() };
  const bool to_isolated{ !op.to.outgoing.Valid() };
  const HalfEdgeId prev_from{ from_isolated ? HalfEdgeId{} : Prev(topology, op.from.outgoing) };
  const HalfEdgeId prev_to{ to_isolated ? HalfEdgeId{} : Prev(topology, op.to.outgoing) };

  const HalfEdgeId out{ NewEdge(topology, op.new_half_edge, op.curve) };
  const HalfEdgeId back{ Twin(out) };
  SetEnds(topology, out, op.from.vertex, op.to.vertex);
  if (from_isolated)
  {
    Link(topology, back, out);
  }
  else
  {
    Link(topology, prev_from, out);
    Link(topology, back, op.from.outgoing);
  }
  if (to_isolated)
  {
    Link(topology, out, back);
  }
  else
  {
    Link(topology, prev_to, back);
    Link(topology, out, op.to.outgoing);
  }
  AssignLoop(topology, out, loop);
  Loop& loop_data{ topology.LoopAt(loop) };
  loop_data.first = out;
  loop_data.isolated_vertex = VertexId{};
  AttachFirstEdge(topology, op.from.vertex, out);
  AttachFirstEdge(topology, op.to.vertex, back);
  topology.DetachInnerLoop(face, ring);
  topology.RemoveLoop(ring);
  // KEMR turns the part around its half-edge's target into the ring.
  return KillEdgeMakeRing{ keep_to ? back : out, ring };
}

jcad::euler::JoinEdgeKillVertex jcad::euler::Apply(topo::Topology& topology,
                                                   const SplitEdgeMakeVertex& op)
{
  const HalfEdgeId half_edge{ op.half_edge };
  const HalfEdgeId twin{ Twin(half_edge) };
  const VertexId end{ topology.Target(half_edge) };
  const HalfEdgeId after{ Next(topology, half_edge) };
  const HalfEdgeId before_twin{ Prev(topology, twin) };
  const CurveId old_curve{ topology.EdgeAt(EdgeOf(half_edge)).curve };

  const VertexId middle{ NewVertex(topology, op.new_vertex, Vertex{ {}, {}, op.point }) };
  const HalfEdgeId split{ NewEdge(topology, op.new_half_edge, op.new_curve) };
  const HalfEdgeId split_twin{ Twin(split) };
  SetEnds(topology, split, middle, end);
  topology.HalfEdgeAt(twin).origin = middle;
  topology.HalfEdgeAt(split).loop = topology.HalfEdgeAt(half_edge).loop;
  topology.HalfEdgeAt(split_twin).loop = topology.HalfEdgeAt(twin).loop;
  // When `end` has no other edge, `after` is the twin and the new pair sits between.
  const bool dangling_end{ after == twin };
  Link(topology, half_edge, split);
  Link(topology, split, dangling_end ? split_twin : after);
  Link(topology, dangling_end ? split : before_twin, split_twin);
  Link(topology, split_twin, twin);

  topology.EdgeAt(EdgeOf(half_edge)).curve = op.curve;
  topology.VertexAt(middle).outgoing = split;
  ReplaceOutgoing(topology, end, twin, split_twin);
  return JoinEdgeKillVertex{ half_edge, old_curve };
}

jcad::euler::SplitEdgeMakeVertex jcad::euler::Apply(topo::Topology& topology,
                                                    const JoinEdgeKillVertex& op)
{
  const HalfEdgeId half_edge{ op.half_edge };
  const HalfEdgeId twin{ Twin(half_edge) };
  const VertexId middle{ topology.Target(half_edge) };
  const HalfEdgeId joined{ Next(topology, half_edge) };
  const HalfEdgeId joined_twin{ Twin(joined) };
  JCAD_ASSERT(joined != twin, "vertex {} has a single edge", middle.Value());
  JCAD_ASSERT(Next(topology, joined_twin) == twin,
              "vertex {} has more than two edges",
              middle.Value());
  const VertexId end{ topology.Target(joined) };
  JCAD_ASSERT(end != topology.HalfEdgeAt(half_edge).origin, "JEKV cannot make a self-loop edge");

  const HalfEdgeId after{ Next(topology, joined) };
  const HalfEdgeId before_twin{ Prev(topology, joined_twin) };
  const CurveId old_curve{ topology.EdgeAt(EdgeOf(half_edge)).curve };
  const CurveId joined_curve{ topology.EdgeAt(EdgeOf(joined)).curve };
  const topo::GeomPointId point{ topology.VertexAt(middle).point };

  const bool dangling_end{ after == joined_twin };
  Link(topology, half_edge, dangling_end ? twin : after);
  Link(topology, dangling_end ? half_edge : before_twin, twin);
  topology.HalfEdgeAt(twin).origin = end;
  Loop& loop{ topology.LoopAt(topology.HalfEdgeAt(half_edge).loop) };
  if (loop.first == joined || loop.first == joined_twin)
    loop.first = half_edge;
  Loop& twin_loop{ topology.LoopAt(topology.HalfEdgeAt(twin).loop) };
  if (twin_loop.first == joined || twin_loop.first == joined_twin)
    twin_loop.first = twin;
  ReplaceOutgoing(topology, end, joined_twin, twin);

  topology.EdgeAt(EdgeOf(half_edge)).curve = op.curve;
  topology.RemoveEdge(EdgeOf(joined));
  topology.RemoveVertex(middle);
  return SplitEdgeMakeVertex{ half_edge, point, old_curve, joined_curve, middle, joined };
}

jcad::euler::MoveRing jcad::euler::Apply(topo::Topology& topology, const MoveRing& op)
{
  JCAD_ASSERT(!topology.IsOuterLoop(op.loop), "only inner loops can move");
  const FaceId from{ topology.LoopAt(op.loop).face };
  topology.DetachInnerLoop(from, op.loop);
  topology.AttachInnerLoop(op.to, op.loop);
  topology.LoopAt(op.loop).face = op.to;
  return MoveRing{ op.loop, from };
}

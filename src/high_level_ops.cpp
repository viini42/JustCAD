#include "euler/high_level_ops.hpp"

#include "core/assert.hpp"
#include "euler/euler_op.hpp"
#include "euler/history.hpp"
#include "topo/half_edge.hpp"
#include "topo/topology.hpp"

#include <variant>
#include <vector>

namespace
{

  using jcad::euler::History;
  using jcad::topo::FaceId;
  using jcad::topo::HalfEdgeId;
  using jcad::topo::LoopId;
  using jcad::topo::Topology;
  using jcad::topo::Twin;

  LoopId LoopOfHalfEdge(const Topology& topology, HalfEdgeId halfEdge)
  {
    return topology.HalfEdgeAt(halfEdge).loop;
  }

  /** The half-edge whose face KEF should kill, or invalid if neither side can go. */
  HalfEdgeId FaceSideToKill(const Topology& topology, HalfEdgeId halfEdge)
  {
    if (topology.IsOuterLoop(LoopOfHalfEdge(topology, halfEdge)))
      return halfEdge;
    if (topology.IsOuterLoop(LoopOfHalfEdge(topology, Twin(halfEdge))))
      return Twin(halfEdge);
    return HalfEdgeId{};
  }

  void MergeFaces(History& history, HalfEdgeId killedSide)
  {
    const Topology& topology{ history.GetTopology() };
    const FaceId killed{ topology.LoopAt(LoopOfHalfEdge(topology, killedSide)).face };
    const FaceId kept{ topology.LoopAt(LoopOfHalfEdge(topology, Twin(killedSide))).face };
    const std::vector<LoopId> inner{ topology.FaceAt(killed).inner };
    for (const LoopId loop : inner)
      history.Apply(jcad::euler::MoveRing{ loop, kept });
    history.Apply(jcad::euler::KillEdgeFace{ killedSide });
  }

} // namespace

jcad::topo::VertexId
jcad::euler::AddIsolatedVertex(History& history, topo::FaceId face, topo::GeomPointId point)
{
  const EulerOp inverse{ history.Apply(MakeVertexRing{ face, point, {}, {} }) };
  return std::get<KillVertexRing>(inverse).vertex;
}

jcad::topo::HalfEdgeId jcad::euler::AddDanglingEdge(History& history,
                                                    const Corner& from,
                                                    topo::GeomPointId point,
                                                    topo::CurveId curve)
{
  const EulerOp inverse{ history.Apply(MakeEdgeVertex{ from, point, curve, {}, {} }) };
  return std::get<KillEdgeVertex>(inverse).half_edge;
}

jcad::euler::AddedEdge
jcad::euler::AddEdge(History& history, const Corner& from, const Corner& to, topo::CurveId curve)
{
  const Topology& topology{ history.GetTopology() };
  if (LoopOf(topology, from) == LoopOf(topology, to))
  {
    const EulerOp inverse{ history.Apply(MakeEdgeFace{ from, to, curve, {}, {}, {} }) };
    const HalfEdgeId half_edge{ std::get<KillEdgeFace>(inverse).half_edge };
    return AddedEdge{ half_edge, topology.LoopAt(LoopOfHalfEdge(topology, half_edge)).face };
  }
  const EulerOp inverse{ history.Apply(MakeEdgeKillRing{ from, to, curve, {} }) };
  const HalfEdgeId recorded{ std::get<KillEdgeMakeRing>(inverse).half_edge };
  const bool runs_forward{ topology.HalfEdgeAt(recorded).origin == from.vertex };
  return AddedEdge{ runs_forward ? recorded : Twin(recorded), FaceId{} };
}

jcad::topo::VertexId jcad::euler::SplitEdge(History& history,
                                            topo::HalfEdgeId halfEdge,
                                            topo::GeomPointId point,
                                            const SplitCurves& curves)
{
  history.Apply(SplitEdgeMakeVertex{ halfEdge, point, curves.before, curves.after, {}, {} });
  return history.GetTopology().Target(halfEdge);
}

void jcad::euler::MergeEdges(History& history, topo::HalfEdgeId into, topo::CurveId curve)
{
  history.Apply(JoinEdgeKillVertex{ into, curve });
}

void jcad::euler::RemoveEdge(History& history, topo::HalfEdgeId halfEdge)
{
  const Topology& topology{ history.GetTopology() };
  if (LoopOfHalfEdge(topology, halfEdge) == LoopOfHalfEdge(topology, Twin(halfEdge)))
  {
    history.Apply(KillEdgeMakeRing{ halfEdge, {} });
    return;
  }
  const HalfEdgeId killed_side{ FaceSideToKill(topology, halfEdge) };
  JCAD_ASSERT(killed_side.Valid(), "neither side of half-edge {} bounds a face", halfEdge.Value());
  history.Begin();
  MergeFaces(history, killed_side);
  history.Commit();
}

void jcad::euler::RemoveVertex(History& history, topo::VertexId vertex)
{
  const Topology& topology{ history.GetTopology() };
  history.Begin();
  while (topology.VertexAt(vertex).outgoing.Valid())
  {
    const HalfEdgeId incoming{ Twin(topology.VertexAt(vertex).outgoing) };
    if (topology.HalfEdgeAt(incoming).next == Twin(incoming))
    {
      // Last edge: KEV kills the vertex together with it.
      history.Apply(KillEdgeVertex{ incoming });
      history.Commit();
      return;
    }
    RemoveEdge(history, incoming);
  }
  history.Apply(KillVertexRing{ vertex });
  history.Commit();
}

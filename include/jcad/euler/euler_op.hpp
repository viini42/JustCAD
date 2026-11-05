#pragma once

#include "euler/corner.hpp"
#include "topo/id.hpp"

#include <variant>

namespace jcad::topo
{
  class Topology;
}

namespace jcad::euler
{

  // Low-level Euler operators, in the style of Mantyla. Each one is a plain record
  // of its arguments; Apply() performs it and returns the record of its inverse,
  // which is the basis for undo/redo. The `new_*` fields name the Ids to create;
  // leave them invalid to get fresh Ids (inverse records fill them in, so undoing a
  // kill restores exactly the Ids it removed). No geometry is computed here: points
  // and curves are opaque Ids.

  /** MVR: makes an isolated vertex, as a new inner loop (ring) of `face`. */
  struct MakeVertexRing
  {
    topo::FaceId face{};
    topo::GeomPointId point{};
    topo::VertexId new_vertex{};
    topo::LoopId new_loop{};
  };

  /** KVR: kills a vertex that has no edges, together with its ring (an inner loop). */
  struct KillVertexRing
  {
    topo::VertexId vertex{};
  };

  /** MEV: makes an edge from the corner `from` to a new vertex (a dangling edge). */
  struct MakeEdgeVertex
  {
    Corner from{};
    topo::GeomPointId point{};
    topo::CurveId curve{};
    topo::VertexId new_vertex{};
    topo::HalfEdgeId new_half_edge{};
  };

  /** KEV: kills `half_edge`'s edge and its target vertex, which must have no other edge. */
  struct KillEdgeVertex
  {
    topo::HalfEdgeId half_edge{};
  };

  /**
   * MEF: makes an edge between two different vertices whose corners lie in the same
   * loop, splitting the loop's face. The new half-edge runs from `from` to `to`; its
   * side of the old loop becomes the outer loop of a new face, and the other side
   * stays with the old face. Inner loops all stay in the old face (move them with
   * MoveRing when they belong to the new one).
   */
  struct MakeEdgeFace
  {
    Corner from{};
    Corner to{};
    topo::CurveId curve{};
    topo::HalfEdgeId new_half_edge{};
    topo::FaceId new_face{};
    topo::LoopId new_loop{};
  };

  /**
   * KEF: kills `half_edge`'s edge and the face on its left, merging that face into
   * the one on the other side. The face to kill must be bounded, have `half_edge` on
   * its outer loop and no inner loops.
   */
  struct KillEdgeFace
  {
    topo::HalfEdgeId half_edge{};
  };

  /**
   * KEMR: kills an edge whose two half-edges are in the same loop, splitting that
   * loop in two. The part after `half_edge` (around its target) becomes a new inner
   * loop of the same face; the part around its origin keeps the old loop.
   */
  struct KillEdgeMakeRing
  {
    topo::HalfEdgeId half_edge{};
    topo::LoopId new_loop{};
  };

  /**
   * MEKR: makes an edge joining two different loops of the same face, from `from`
   * to `to`. The loop of `to` must be an inner loop; it is killed and merged into
   * the loop of `from`.
   */
  struct MakeEdgeKillRing
  {
    Corner from{};
    Corner to{};
    topo::CurveId curve{};
    topo::HalfEdgeId new_half_edge{};
  };

  /** Moves an inner loop to another face. Not an Euler operator, but its own inverse. */
  struct MoveRing
  {
    topo::LoopId loop{};
    topo::FaceId to{};
  };

  using EulerOp = std::variant<MakeVertexRing,
                               KillVertexRing,
                               MakeEdgeVertex,
                               KillEdgeVertex,
                               MakeEdgeFace,
                               KillEdgeFace,
                               KillEdgeMakeRing,
                               MakeEdgeKillRing,
                               MoveRing>;

  /** Performs the operator and returns its inverse. Preconditions are asserted. */
  EulerOp Apply(topo::Topology& topology, const EulerOp& op);

  KillVertexRing Apply(topo::Topology& topology, const MakeVertexRing& op);
  MakeVertexRing Apply(topo::Topology& topology, const KillVertexRing& op);
  KillEdgeVertex Apply(topo::Topology& topology, const MakeEdgeVertex& op);
  MakeEdgeVertex Apply(topo::Topology& topology, const KillEdgeVertex& op);
  KillEdgeFace Apply(topo::Topology& topology, const MakeEdgeFace& op);
  MakeEdgeFace Apply(topo::Topology& topology, const KillEdgeFace& op);
  MakeEdgeKillRing Apply(topo::Topology& topology, const KillEdgeMakeRing& op);
  KillEdgeMakeRing Apply(topo::Topology& topology, const MakeEdgeKillRing& op);
  MoveRing Apply(topo::Topology& topology, const MoveRing& op);

  /** The loop a corner lies in: the loop of its outgoing half-edge, or the vertex's ring. */
  [[nodiscard]] topo::LoopId LoopOf(const topo::Topology& topology, const Corner& corner);

} // namespace jcad::euler

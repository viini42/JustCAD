#pragma once

#include "euler/corner.hpp"
#include "topo/id.hpp"

namespace jcad::euler
{

  class History;

  // High-level Euler operators: modeling steps composed of low-level operators.
  // Each one is a single undo entry (a transaction) in the History. They are purely
  // topological: the caller, which knows the geometry, chooses the corners and the
  // half-edge orientations, and moves inner loops when a new face encloses them.

  /** Result of AddEdge(). */
  struct AddedEdge
  {
    /** Runs from the `from` corner to the `to` corner. */
    topo::HalfEdgeId half_edge{};
    /** The face split off on `half_edge`'s left, if the edge closed a loop. */
    topo::FaceId new_face{};
  };

  /** Curves of the two pieces of a split edge, in the split half-edge's direction. */
  struct SplitCurves
  {
    topo::CurveId before{};
    topo::CurveId after{};
  };

  topo::VertexId AddIsolatedVertex(History& history, topo::FaceId face, topo::GeomPointId point);

  /** Adds an edge from `from` to a new vertex; returns the half-edge leaving `from`. */
  topo::HalfEdgeId AddDanglingEdge(History& history,
                                   const Corner& from,
                                   topo::GeomPointId point,
                                   topo::CurveId curve);

  /**
   * Adds an edge between two existing, different vertices. Corners in the same loop
   * split its face (MEF); corners in different loops of one face join them (MEKR).
   */
  AddedEdge AddEdge(History& history, const Corner& from, const Corner& to, topo::CurveId curve);

  /** Splits `halfEdge` at a new vertex, which is returned. */
  topo::VertexId SplitEdge(History& history,
                           topo::HalfEdgeId halfEdge,
                           topo::GeomPointId point,
                           const SplitCurves& curves);

  /**
   * Removes the vertex at the end of `into`, which must have exactly two edges,
   * joining them into `into`'s edge. `curve` runs along that edge's first half-edge.
   */
  void MergeEdges(History& history, topo::HalfEdgeId into, topo::CurveId curve);

  /**
   * Removes an edge; its vertices stay. If the two sides bound different faces, the
   * face on `halfEdge`'s left (or, if that one is not bounded by its outer loop, the
   * face on the right) is merged into the other one, inner loops included. If both
   * sides are in the same loop, the part around `halfEdge`'s target becomes an inner
   * loop.
   */
  void RemoveEdge(History& history, topo::HalfEdgeId halfEdge);

  /** Removes a vertex together with all its edges. */
  void RemoveVertex(History& history, topo::VertexId vertex);

} // namespace jcad::euler

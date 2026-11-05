#pragma once

#include "topo/id.hpp"

namespace jcad::topo
{

  /**
   * One side of an edge. Its face lies on its left, so the outer loop of a bounded
   * face runs counter-clockwise and the loops around holes run clockwise.
   *
   * The two half-edges of edge e have the Ids 2e and 2e + 1, so the twin and the
   * edge are computed rather than stored.
   */
  struct HalfEdge
  {
    VertexId origin{};
    HalfEdgeId next{};
    HalfEdgeId prev{};
    LoopId loop{};

    bool operator==(const HalfEdge&) const = default;
  };

  [[nodiscard]] constexpr HalfEdgeId Twin(HalfEdgeId halfEdge) noexcept
  {
    return HalfEdgeId{ halfEdge.Value() ^ 1U };
  }

  [[nodiscard]] constexpr EdgeId EdgeOf(HalfEdgeId halfEdge) noexcept
  {
    return EdgeId{ halfEdge.Value() >> 1U };
  }

  /** The half-edge that runs in the direction of the edge's curve. */
  [[nodiscard]] constexpr HalfEdgeId FirstHalfEdge(EdgeId edge) noexcept
  {
    return HalfEdgeId{ edge.Value() * 2U };
  }

  [[nodiscard]] constexpr HalfEdgeId SecondHalfEdge(EdgeId edge) noexcept
  {
    return HalfEdgeId{ edge.Value() * 2U + 1U };
  }

} // namespace jcad::topo

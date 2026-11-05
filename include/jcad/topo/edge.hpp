#pragma once

#include "topo/id.hpp"

namespace jcad::topo
{

  /**
   * Its half-edges are FirstHalfEdge(e) and SecondHalfEdge(e); the curve runs from
   * the origin of the first to the origin of the second. Edges never start and end
   * at the same vertex: a closed curve is split into at least two edges.
   */
  struct Edge
  {
    CurveId curve{};

    bool operator==(const Edge&) const = default;
  };

} // namespace jcad::topo

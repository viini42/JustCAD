#pragma once

#include "topo/id.hpp"

namespace jcad::topo
{

  /**
   * A vertex has either an outgoing half-edge or, when no edge touches it, a ring:
   * the loop it forms on its own inside a face.
   */
  struct Vertex
  {
    HalfEdgeId outgoing{};
    LoopId ring{};
    GeomPointId point{};
  };

} // namespace jcad::topo

#pragma once

#include "topo/id.hpp"

namespace jcad::euler
{

  /**
   * A place around a vertex where a new edge can be attached: just before the
   * half-edge `outgoing`, which must leave `vertex`. A vertex without edges has a
   * single corner, with an invalid `outgoing`.
   */
  struct Corner
  {
    topo::VertexId vertex{};
    topo::HalfEdgeId outgoing{};
  };

} // namespace jcad::euler

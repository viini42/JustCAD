#pragma once

#include "topo/id.hpp"

namespace jcad::topo
{

  /**
   * A closed cycle of half-edges bounding a face. A loop around a vertex that has no
   * edges has no half-edges and names that vertex instead.
   */
  struct Loop
  {
    HalfEdgeId first{};
    VertexId isolated_vertex{};
    FaceId face{};
  };

} // namespace jcad::topo

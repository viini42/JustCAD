#pragma once

#include "topo/id.hpp"

#include <vector>

namespace jcad::topo
{

  /**
   * A region of the plane. The unbounded face has no outer loop. Inner loops are the
   * boundaries of everything drawn inside the face (holes and separate drawings),
   * kept sorted by Id.
   */
  struct Face
  {
    LoopId outer{};
    std::vector<LoopId> inner;

    bool operator==(const Face&) const = default;
  };

} // namespace jcad::topo

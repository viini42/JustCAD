#pragma once

#include "topo/edge.hpp"
#include "topo/face.hpp"
#include "topo/half_edge.hpp"
#include "topo/id.hpp"
#include "topo/loop.hpp"
#include "topo/pool.hpp"
#include "topo/vertex.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace jcad::topo
{

  /**
   * Half-edge data structure of a planar subdivision.
   *
   * It always holds the unbounded face, which has no outer loop; everything drawn
   * lives inside it. Entities are created and linked by the Euler operators
   * (euler/euler_op.hpp). The mutable accessors and the Add/Remove functions exist
   * for those operators: on their own they keep the pools consistent but not the
   * topology, which is what Validate() checks.
   */
  class Topology
  {
  public:
    Topology();

    [[nodiscard]] FaceId UnboundedFace() const noexcept;

    [[nodiscard]] std::size_t VertexCount() const noexcept;
    [[nodiscard]] std::size_t EdgeCount() const noexcept;
    [[nodiscard]] std::size_t LoopCount() const noexcept;
    [[nodiscard]] std::size_t FaceCount() const noexcept;
    [[nodiscard]] std::size_t InnerLoopCount() const;

    [[nodiscard]] bool Contains(VertexId vertex) const noexcept;
    [[nodiscard]] bool Contains(HalfEdgeId halfEdge) const noexcept;
    [[nodiscard]] bool Contains(EdgeId edge) const noexcept;
    [[nodiscard]] bool Contains(LoopId loop) const noexcept;
    [[nodiscard]] bool Contains(FaceId face) const noexcept;

    [[nodiscard]] std::vector<VertexId> VertexIds() const;
    [[nodiscard]] std::vector<EdgeId> EdgeIds() const;
    [[nodiscard]] std::vector<LoopId> LoopIds() const;
    [[nodiscard]] std::vector<FaceId> FaceIds() const;

    [[nodiscard]] const Vertex& VertexAt(VertexId vertex) const;
    [[nodiscard]] Vertex& VertexAt(VertexId vertex);
    [[nodiscard]] const HalfEdge& HalfEdgeAt(HalfEdgeId halfEdge) const;
    [[nodiscard]] HalfEdge& HalfEdgeAt(HalfEdgeId halfEdge);
    [[nodiscard]] const Edge& EdgeAt(EdgeId edge) const;
    [[nodiscard]] Edge& EdgeAt(EdgeId edge);
    [[nodiscard]] const Loop& LoopAt(LoopId loop) const;
    [[nodiscard]] Loop& LoopAt(LoopId loop);
    [[nodiscard]] const Face& FaceAt(FaceId face) const;
    [[nodiscard]] Face& FaceAt(FaceId face);

    /** The vertex a half-edge points to (the origin of its twin). */
    [[nodiscard]] VertexId Target(HalfEdgeId halfEdge) const;

    /** The half-edges of a loop in order, starting at its first; empty for a vertex ring. */
    [[nodiscard]] std::vector<HalfEdgeId> LoopHalfEdges(LoopId loop) const;

    /** The half-edges leaving a vertex, in rotation order around it. */
    [[nodiscard]] std::vector<HalfEdgeId> OutgoingHalfEdges(VertexId vertex) const;

    [[nodiscard]] bool IsOuterLoop(LoopId loop) const;

    [[nodiscard]] VertexId AddVertex(Vertex vertex);
    void AddVertexAt(VertexId id, Vertex vertex);
    void RemoveVertex(VertexId vertex);

    /** Also allocates the edge's two half-edges, unlinked. */
    [[nodiscard]] EdgeId AddEdge(Edge edge);
    void AddEdgeAt(EdgeId id, Edge edge);
    void RemoveEdge(EdgeId edge);

    [[nodiscard]] LoopId AddLoop(Loop loop);
    void AddLoopAt(LoopId id, Loop loop);
    void RemoveLoop(LoopId loop);

    [[nodiscard]] FaceId AddFace(Face face);
    void AddFaceAt(FaceId id, Face face);
    void RemoveFace(FaceId face);

    /** Adds `loop` to the sorted inner loops of `face`. Does not change loop.face. */
    void AttachInnerLoop(FaceId face, LoopId loop);
    void DetachInnerLoop(FaceId face, LoopId loop);

    /**
     * Checks every topological invariant: twin/next/prev links, loop and face
     * back-references, a single fan of edges around each vertex and the
     * Euler-Poincare formula V - E + F = 1 + H (H = number of inner loops). Returns a
     * description of the first violation found.
     */
    [[nodiscard]] std::expected<void, std::string> Validate() const;

    /**
     * Structural equality: same alive Ids with the same links. Which half-edge a loop
     * starts at, or which one a vertex names as outgoing, does not matter.
     */
    [[nodiscard]] bool operator==(const Topology& other) const;

  private:
    using ValidationResult = std::expected<void, std::string>;

    [[nodiscard]] ValidationResult ValidateFaces() const;
    [[nodiscard]] ValidationResult ValidateLoops() const;
    [[nodiscard]] ValidationResult ValidateLoopCycle(LoopId loop) const;
    [[nodiscard]] ValidationResult ValidateHalfEdges() const;
    [[nodiscard]] ValidationResult ValidateHalfEdge(HalfEdgeId halfEdge) const;
    [[nodiscard]] ValidationResult ValidateVertices() const;
    [[nodiscard]] ValidationResult ValidateVertexFan(VertexId vertex) const;
    [[nodiscard]] ValidationResult ValidateEulerFormula() const;

    [[nodiscard]] bool SameVertices(const Topology& other) const;
    [[nodiscard]] bool SameEdges(const Topology& other) const;
    [[nodiscard]] bool SameLoops(const Topology& other) const;

    void ResetHalfEdges(EdgeId edge);

    Pool<VertexId, Vertex> m_vertices;
    Pool<EdgeId, Edge> m_edges;
    std::vector<HalfEdge> m_half_edges;
    Pool<LoopId, Loop> m_loops;
    Pool<FaceId, Face> m_faces;
    FaceId m_unbounded_face;
  };

} // namespace jcad::topo

#include "topo/topology.hpp"

#include "core/assert.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <utility>

namespace
{

  template <typename... Args>
  std::unexpected<std::string> Fail(std::format_string<Args...> fmt, Args&&... args)
  {
    return std::unexpected{ std::format(fmt, std::forward<Args>(args)...) };
  }

  std::int64_t Signed(std::size_t value)
  {
    return static_cast<std::int64_t>(value);
  }

} // namespace

jcad::topo::Topology::Topology() : m_unbounded_face{ m_faces.Allocate(Face{}) } {}

jcad::topo::FaceId jcad::topo::Topology::UnboundedFace() const noexcept
{
  return m_unbounded_face;
}

std::size_t jcad::topo::Topology::VertexCount() const noexcept
{
  return m_vertices.Size();
}

std::size_t jcad::topo::Topology::EdgeCount() const noexcept
{
  return m_edges.Size();
}

std::size_t jcad::topo::Topology::LoopCount() const noexcept
{
  return m_loops.Size();
}

std::size_t jcad::topo::Topology::FaceCount() const noexcept
{
  return m_faces.Size();
}

std::size_t jcad::topo::Topology::InnerLoopCount() const
{
  std::size_t count{ 0 };
  for (const FaceId face : m_faces.AliveIds())
    count += m_faces.Get(face).inner.size();
  return count;
}

bool jcad::topo::Topology::Contains(VertexId vertex) const noexcept
{
  return m_vertices.IsAlive(vertex);
}

bool jcad::topo::Topology::Contains(HalfEdgeId halfEdge) const noexcept
{
  return halfEdge.Valid() && m_edges.IsAlive(EdgeOf(halfEdge));
}

bool jcad::topo::Topology::Contains(EdgeId edge) const noexcept
{
  return m_edges.IsAlive(edge);
}

bool jcad::topo::Topology::Contains(LoopId loop) const noexcept
{
  return m_loops.IsAlive(loop);
}

bool jcad::topo::Topology::Contains(FaceId face) const noexcept
{
  return m_faces.IsAlive(face);
}

std::vector<jcad::topo::VertexId> jcad::topo::Topology::VertexIds() const
{
  return m_vertices.AliveIds();
}

std::vector<jcad::topo::EdgeId> jcad::topo::Topology::EdgeIds() const
{
  return m_edges.AliveIds();
}

std::vector<jcad::topo::LoopId> jcad::topo::Topology::LoopIds() const
{
  return m_loops.AliveIds();
}

std::vector<jcad::topo::FaceId> jcad::topo::Topology::FaceIds() const
{
  return m_faces.AliveIds();
}

const jcad::topo::Vertex& jcad::topo::Topology::VertexAt(VertexId vertex) const
{
  return m_vertices.Get(vertex);
}

jcad::topo::Vertex& jcad::topo::Topology::VertexAt(VertexId vertex)
{
  return m_vertices.Get(vertex);
}

const jcad::topo::HalfEdge& jcad::topo::Topology::HalfEdgeAt(HalfEdgeId halfEdge) const
{
  JCAD_ASSERT(Contains(halfEdge), "half-edge {} is not alive", halfEdge.Value());
  return m_half_edges[halfEdge.Value()];
}

jcad::topo::HalfEdge& jcad::topo::Topology::HalfEdgeAt(HalfEdgeId halfEdge)
{
  JCAD_ASSERT(Contains(halfEdge), "half-edge {} is not alive", halfEdge.Value());
  return m_half_edges[halfEdge.Value()];
}

const jcad::topo::Edge& jcad::topo::Topology::EdgeAt(EdgeId edge) const
{
  return m_edges.Get(edge);
}

jcad::topo::Edge& jcad::topo::Topology::EdgeAt(EdgeId edge)
{
  return m_edges.Get(edge);
}

const jcad::topo::Loop& jcad::topo::Topology::LoopAt(LoopId loop) const
{
  return m_loops.Get(loop);
}

jcad::topo::Loop& jcad::topo::Topology::LoopAt(LoopId loop)
{
  return m_loops.Get(loop);
}

const jcad::topo::Face& jcad::topo::Topology::FaceAt(FaceId face) const
{
  return m_faces.Get(face);
}

jcad::topo::Face& jcad::topo::Topology::FaceAt(FaceId face)
{
  return m_faces.Get(face);
}

jcad::topo::VertexId jcad::topo::Topology::Target(HalfEdgeId halfEdge) const
{
  return HalfEdgeAt(Twin(halfEdge)).origin;
}

std::vector<jcad::topo::HalfEdgeId> jcad::topo::Topology::LoopHalfEdges(LoopId loop) const
{
  std::vector<HalfEdgeId> half_edges;
  const HalfEdgeId first{ LoopAt(loop).first };
  if (!first.Valid())
    return half_edges;
  HalfEdgeId current{ first };
  do
  {
    half_edges.push_back(current);
    JCAD_ASSERT(half_edges.size() <= m_half_edges.size(), "loop {} does not close", loop.Value());
    current = HalfEdgeAt(current).next;
  } while (current != first);
  return half_edges;
}

std::vector<jcad::topo::HalfEdgeId> jcad::topo::Topology::OutgoingHalfEdges(VertexId vertex) const
{
  std::vector<HalfEdgeId> half_edges;
  const HalfEdgeId first{ VertexAt(vertex).outgoing };
  if (!first.Valid())
    return half_edges;
  HalfEdgeId current{ first };
  do
  {
    half_edges.push_back(current);
    JCAD_ASSERT(half_edges.size() <= m_half_edges.size(),
                "fan of {} does not close",
                vertex.Value());
    current = HalfEdgeAt(Twin(current)).next;
  } while (current != first);
  return half_edges;
}

bool jcad::topo::Topology::IsOuterLoop(LoopId loop) const
{
  return FaceAt(LoopAt(loop).face).outer == loop;
}

jcad::topo::VertexId jcad::topo::Topology::AddVertex(Vertex vertex)
{
  return m_vertices.Allocate(vertex);
}

void jcad::topo::Topology::AddVertexAt(VertexId id, Vertex vertex)
{
  m_vertices.AllocateAt(id, vertex);
}

void jcad::topo::Topology::RemoveVertex(VertexId vertex)
{
  m_vertices.Free(vertex);
}

jcad::topo::EdgeId jcad::topo::Topology::AddEdge(Edge edge)
{
  const EdgeId id{ m_edges.Allocate(edge) };
  ResetHalfEdges(id);
  return id;
}

void jcad::topo::Topology::AddEdgeAt(EdgeId id, Edge edge)
{
  m_edges.AllocateAt(id, edge);
  ResetHalfEdges(id);
}

void jcad::topo::Topology::RemoveEdge(EdgeId edge)
{
  m_edges.Free(edge);
  ResetHalfEdges(edge);
}

jcad::topo::LoopId jcad::topo::Topology::AddLoop(Loop loop)
{
  return m_loops.Allocate(loop);
}

void jcad::topo::Topology::AddLoopAt(LoopId id, Loop loop)
{
  m_loops.AllocateAt(id, loop);
}

void jcad::topo::Topology::RemoveLoop(LoopId loop)
{
  m_loops.Free(loop);
}

jcad::topo::FaceId jcad::topo::Topology::AddFace(Face face)
{
  return m_faces.Allocate(std::move(face));
}

void jcad::topo::Topology::AddFaceAt(FaceId id, Face face)
{
  m_faces.AllocateAt(id, std::move(face));
}

void jcad::topo::Topology::RemoveFace(FaceId face)
{
  JCAD_ASSERT(face != m_unbounded_face, "the unbounded face cannot be removed");
  m_faces.Free(face);
}

void jcad::topo::Topology::AttachInnerLoop(FaceId face, LoopId loop)
{
  std::vector<LoopId>& inner{ FaceAt(face).inner };
  const auto position{ std::ranges::lower_bound(inner, loop) };
  JCAD_ASSERT(position == inner.end() || *position != loop,
              "loop {} already attached",
              loop.Value());
  inner.insert(position, loop);
}

void jcad::topo::Topology::DetachInnerLoop(FaceId face, LoopId loop)
{
  std::vector<LoopId>& inner{ FaceAt(face).inner };
  const auto position{ std::ranges::lower_bound(inner, loop) };
  JCAD_ASSERT(position != inner.end() && *position == loop, "loop {} not attached", loop.Value());
  inner.erase(position);
}

std::expected<void, std::string> jcad::topo::Topology::Validate() const
{
  return ValidateFaces()
    .and_then([this] { return ValidateLoops(); })
    .and_then([this] { return ValidateHalfEdges(); })
    .and_then([this] { return ValidateVertices(); })
    .and_then([this] { return ValidateEulerFormula(); });
}

bool jcad::topo::Topology::operator==(const Topology& other) const
{
  if (m_unbounded_face != other.m_unbounded_face)
    return false;
  if (FaceIds() != other.FaceIds())
    return false;
  for (const FaceId face : FaceIds())
  {
    if (FaceAt(face) != other.FaceAt(face))
      return false;
  }
  return SameVertices(other) && SameEdges(other) && SameLoops(other);
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateFaces() const
{
  if (!Contains(m_unbounded_face))
    return Fail("the unbounded face is missing");
  if (FaceAt(m_unbounded_face).outer.Valid())
    return Fail("the unbounded face has an outer loop");
  for (const FaceId face : FaceIds())
  {
    const Face& data{ FaceAt(face) };
    if (face != m_unbounded_face && !Contains(data.outer))
      return Fail("bounded face {} has no outer loop", face.Value());
    if (data.outer.Valid() && LoopAt(data.outer).face != face)
      return Fail("outer loop of face {} points to another face", face.Value());
    if (!std::ranges::is_sorted(data.inner) ||
        std::ranges::adjacent_find(data.inner) != data.inner.end())
      return Fail("inner loops of face {} are not sorted and unique", face.Value());
    const bool all_back{ std::ranges::all_of(data.inner,
                                             [&](LoopId loop) {
                                               return Contains(loop) && loop != data.outer &&
                                                      LoopAt(loop).face == face;
                                             }) };
    if (!all_back)
      return Fail("an inner loop of face {} is dead or points to another face", face.Value());
  }
  return {};
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateLoops() const
{
  std::size_t walked{ 0 };
  for (const LoopId loop : LoopIds())
  {
    const Loop& data{ LoopAt(loop) };
    if (!Contains(data.face))
      return Fail("loop {} has no face", loop.Value());
    const Face& face{ FaceAt(data.face) };
    if (face.outer != loop && !std::ranges::binary_search(face.inner, loop))
      return Fail("loop {} is not listed in its face {}", loop.Value(), data.face.Value());
    const ValidationResult cycle{ ValidateLoopCycle(loop) };
    if (!cycle)
      return cycle;
    if (data.first.Valid())
      walked += LoopHalfEdges(loop).size();
  }
  if (walked != 2 * EdgeCount())
    return Fail("loops hold {} half-edges, expected {}", walked, 2 * EdgeCount());
  return {};
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateLoopCycle(LoopId loop) const
{
  const Loop& data{ LoopAt(loop) };
  if (data.isolated_vertex.Valid())
  {
    if (data.first.Valid())
      return Fail("loop {} has both a vertex and half-edges", loop.Value());
    if (!Contains(data.isolated_vertex) || VertexAt(data.isolated_vertex).ring != loop)
      return Fail("ring {} and its vertex do not point to each other", loop.Value());
    return {};
  }
  if (!Contains(data.first))
    return Fail("loop {} has neither a vertex nor half-edges", loop.Value());
  HalfEdgeId current{ data.first };
  std::size_t steps{ 0 };
  do
  {
    if (!Contains(current) || HalfEdgeAt(current).loop != loop)
      return Fail("half-edge {} in the cycle of loop {} belongs elsewhere",
                  current.Value(),
                  loop.Value());
    if (++steps > 2 * EdgeCount())
      return Fail("loop {} does not close", loop.Value());
    current = HalfEdgeAt(current).next;
  } while (current != data.first);
  return {};
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateHalfEdges() const
{
  for (const EdgeId edge : EdgeIds())
  {
    for (const HalfEdgeId half_edge : { FirstHalfEdge(edge), SecondHalfEdge(edge) })
    {
      const ValidationResult result{ ValidateHalfEdge(half_edge) };
      if (!result)
        return result;
    }
  }
  return {};
}

jcad::topo::Topology::ValidationResult
jcad::topo::Topology::ValidateHalfEdge(HalfEdgeId halfEdge) const
{
  const HalfEdge& data{ HalfEdgeAt(halfEdge) };
  const std::uint32_t id{ halfEdge.Value() };
  if (!Contains(data.origin))
    return Fail("half-edge {} has no origin", id);
  if (data.origin == HalfEdgeAt(Twin(halfEdge)).origin)
    return Fail("half-edge {} starts and ends at the same vertex", id);
  if (!Contains(data.next) || !Contains(data.prev))
    return Fail("half-edge {} has a dead next or prev", id);
  if (HalfEdgeAt(data.next).prev != halfEdge || HalfEdgeAt(data.prev).next != halfEdge)
    return Fail("next/prev of half-edge {} are not inverse", id);
  if (HalfEdgeAt(data.next).origin != Target(halfEdge))
    return Fail("next of half-edge {} does not start where it ends", id);
  if (!Contains(data.loop) || HalfEdgeAt(data.next).loop != data.loop)
    return Fail("half-edge {} and its next are in different loops", id);
  return {};
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateVertices() const
{
  for (const VertexId vertex : VertexIds())
  {
    const Vertex& data{ VertexAt(vertex) };
    if (data.outgoing.Valid() == data.ring.Valid())
      return Fail("vertex {} must have either an outgoing half-edge or a ring", vertex.Value());
    if (data.ring.Valid() && (!Contains(data.ring) || LoopAt(data.ring).isolated_vertex != vertex))
      return Fail("vertex {} and its ring do not point to each other", vertex.Value());
    if (data.outgoing.Valid())
    {
      const ValidationResult fan{ ValidateVertexFan(vertex) };
      if (!fan)
        return fan;
    }
  }
  return {};
}

jcad::topo::Topology::ValidationResult
jcad::topo::Topology::ValidateVertexFan(VertexId vertex) const
{
  const HalfEdgeId first{ VertexAt(vertex).outgoing };
  if (!Contains(first) || HalfEdgeAt(first).origin != vertex)
    return Fail("outgoing half-edge of vertex {} does not start there", vertex.Value());
  std::size_t degree{ 0 };
  for (const EdgeId edge : EdgeIds())
  {
    degree += HalfEdgeAt(FirstHalfEdge(edge)).origin == vertex ? 1U : 0U;
    degree += HalfEdgeAt(SecondHalfEdge(edge)).origin == vertex ? 1U : 0U;
  }
  // Rotating around the vertex must reach every half-edge leaving it; otherwise the
  // vertex joins two separate fans (a non-manifold pinch).
  HalfEdgeId current{ first };
  std::size_t fan{ 0 };
  do
  {
    if (++fan > degree)
      return Fail("fan around vertex {} does not close", vertex.Value());
    current = HalfEdgeAt(Twin(current)).next;
  } while (current != first);
  if (fan != degree)
    return Fail("vertex {} has {} edges but its fan reaches {}", vertex.Value(), degree, fan);
  return {};
}

jcad::topo::Topology::ValidationResult jcad::topo::Topology::ValidateEulerFormula() const
{
  const std::int64_t lhs{ Signed(VertexCount()) - Signed(EdgeCount()) + Signed(FaceCount()) };
  const std::int64_t rhs{ 1 + Signed(InnerLoopCount()) };
  if (lhs != rhs)
    return Fail("Euler-Poincare: V - E + F = {} but 1 + H = {}", lhs, rhs);
  if (LoopCount() != FaceCount() - 1 + InnerLoopCount())
    return Fail("{} loops, but faces list {}", LoopCount(), FaceCount() - 1 + InnerLoopCount());
  return {};
}

bool jcad::topo::Topology::SameVertices(const Topology& other) const
{
  if (VertexIds() != other.VertexIds())
    return false;
  return std::ranges::all_of(VertexIds(),
                             [&](VertexId vertex)
                             {
                               const Vertex& a{ VertexAt(vertex) };
                               const Vertex& b{ other.VertexAt(vertex) };
                               return a.point == b.point && a.ring == b.ring &&
                                      a.outgoing.Valid() == b.outgoing.Valid();
                             });
}

bool jcad::topo::Topology::SameEdges(const Topology& other) const
{
  if (EdgeIds() != other.EdgeIds())
    return false;
  return std::ranges::all_of(
    EdgeIds(),
    [&](EdgeId edge)
    {
      return EdgeAt(edge) == other.EdgeAt(edge) &&
             HalfEdgeAt(FirstHalfEdge(edge)) == other.HalfEdgeAt(FirstHalfEdge(edge)) &&
             HalfEdgeAt(SecondHalfEdge(edge)) == other.HalfEdgeAt(SecondHalfEdge(edge));
    });
}

bool jcad::topo::Topology::SameLoops(const Topology& other) const
{
  if (LoopIds() != other.LoopIds())
    return false;
  return std::ranges::all_of(LoopIds(),
                             [&](LoopId loop)
                             {
                               const Loop& a{ LoopAt(loop) };
                               const Loop& b{ other.LoopAt(loop) };
                               return a.face == b.face && a.isolated_vertex == b.isolated_vertex &&
                                      a.first.Valid() == b.first.Valid();
                             });
}

void jcad::topo::Topology::ResetHalfEdges(EdgeId edge)
{
  const std::size_t needed{ 2 * m_edges.Capacity() };
  if (m_half_edges.size() < needed)
    m_half_edges.resize(needed);
  m_half_edges[FirstHalfEdge(edge).Value()] = HalfEdge{};
  m_half_edges[SecondHalfEdge(edge).Value()] = HalfEdge{};
}

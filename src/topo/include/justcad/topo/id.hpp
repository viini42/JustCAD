#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace justcad::topo {

/**
 * Strong, stable identifier of a topological entity.
 *
 * Entities live in pools and refer to each other by Id rather than by pointer,
 * which keeps undo/redo, serialization and validation simple, and makes mixing
 * up, say, a VertexId with an EdgeId a compile error.
 */
template <typename Tag>
class Id {
public:
    using ValueType = std::uint32_t;

    static constexpr ValueType INVALID{std::numeric_limits<ValueType>::max()};

    constexpr Id() noexcept = default;
    constexpr explicit Id(ValueType value) noexcept : m_value{value} {}

    [[nodiscard]] constexpr ValueType Value() const noexcept { return m_value; }
    [[nodiscard]] constexpr bool Valid() const noexcept { return m_value != INVALID; }

    constexpr auto operator<=>(const Id&) const = default;

private:
    ValueType m_value{INVALID};
};

using VertexId = Id<struct VertexTag>;
using HalfEdgeId = Id<struct HalfEdgeTag>;
using EdgeId = Id<struct EdgeTag>;
using LoopId = Id<struct LoopTag>;
using FaceId = Id<struct FaceTag>;

/** Opaque references to geometry stored outside the topology. */
using GeomPointId = Id<struct GeomPointTag>;
using CurveId = Id<struct CurveTag>;

}  // namespace justcad::topo

template <typename Tag>
struct std::hash<justcad::topo::Id<Tag>> {
    std::size_t operator()(const justcad::topo::Id<Tag>& id) const noexcept {
        return std::hash<typename justcad::topo::Id<Tag>::ValueType>{}(id.Value());
    }
};

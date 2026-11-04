#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>

namespace justcad::topo {

/// Identificador forte e estável de uma entidade topológica.
///
/// Entidades vivem em pools e se referenciam por Id, não por ponteiro: isso
/// facilita undo/redo, serialização e validação, e impede misturar, por
/// exemplo, um VertexId com um EdgeId em tempo de compilação.
template <class Tag>
class Id {
public:
    using value_type = std::uint32_t;
    static constexpr value_type kInvalid = std::numeric_limits<value_type>::max();

    constexpr Id() noexcept = default;
    constexpr explicit Id(value_type v) noexcept : m_value(v) {}

    [[nodiscard]] constexpr value_type value() const noexcept { return m_value; }
    [[nodiscard]] constexpr bool valid() const noexcept { return m_value != kInvalid; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    constexpr auto operator<=>(const Id&) const = default;

private:
    value_type m_value = kInvalid;
};

using VertexId = Id<struct VertexTag>;
using HalfEdgeId = Id<struct HalfEdgeTag>;
using EdgeId = Id<struct EdgeTag>;
using LoopId = Id<struct LoopTag>;
using FaceId = Id<struct FaceTag>;

/// Referência opaca para a geometria (ponto/curva) guardada fora da topologia.
using GeomPointId = Id<struct GeomPointTag>;
using CurveId = Id<struct CurveTag>;

}  // namespace justcad::topo

template <class Tag>
struct std::hash<justcad::topo::Id<Tag>> {
    std::size_t operator()(const justcad::topo::Id<Tag>& id) const noexcept {
        return std::hash<typename justcad::topo::Id<Tag>::value_type>{}(id.value());
    }
};

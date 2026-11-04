#pragma once

namespace justcad::geom {

/// Resultado de um predicado geométrico. Toda decisão topológica deve ser
/// tomada a partir de um Sign devolvido pelo kernel, nunca comparando
/// escalares diretamente com um epsilon.
enum class Sign : int { Negative = -1, Zero = 0, Positive = 1 };

[[nodiscard]] constexpr Sign operator-(Sign s) noexcept {
    return static_cast<Sign>(-static_cast<int>(s));
}

}  // namespace justcad::geom

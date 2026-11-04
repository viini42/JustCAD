#pragma once

#include <concepts>

#include "justcad/geom/sign.hpp"

namespace justcad::geom {

/// Kernel numérico: concentra o tipo escalar e os *predicados* geométricos.
///
/// As camadas acima (euler/high, model) são parametrizadas por um Kernel, de
/// modo que trocar `double` com tolerância por aritmética exata (ou filtrada)
/// seja só trocar o tipo. Para isso valer, toda decisão topológica (lado de
/// uma reta, coincidência de pontos, ordenação) deve passar por estes
/// predicados, e nunca por comparações soltas com epsilon.
///
/// Construções (ponto de interseção, ponto em t) ficam fora do kernel: num
/// kernel exato elas podem ser caras ou não representáveis.
template <class K>
concept Kernel =
    requires {
        typename K::Scalar;
        typename K::Point2;
        typename K::Vec2;
    } &&
    requires(const typename K::Point2& a, const typename K::Point2& b,
             const typename K::Point2& c, const typename K::Scalar& s) {
        /// Sinal de um escalar (Zero dentro da tolerância, se houver).
        { K::sign(s) } -> std::same_as<Sign>;
        /// Positive: c à esquerda de a->b; Negative: à direita; Zero: colinear.
        { K::orientation(a, b, c) } -> std::same_as<Sign>;
        /// Pontos coincidentes.
        { K::equal(a, b) } -> std::same_as<bool>;
        /// Ordem lexicográfica (x, depois y).
        { K::compareXY(a, b) } -> std::same_as<Sign>;
        /// c está sobre o segmento fechado [a, b].
        { K::onSegment(a, b, c) } -> std::same_as<bool>;
    };

}  // namespace justcad::geom

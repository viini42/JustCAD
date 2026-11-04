# JustCAD

Modelador 2D para desenhar primitivas geométricas (pontos, linhas, poligonais,
curvas Bézier) e regiões fechadas (retângulos, polígonos regulares, círculos,
elipses e regiões delimitadas por primitivas).

Internamente, o modelo é uma estrutura topológica **half-edge** manipulada
apenas por **operadores de Euler**, organizada em camadas com dependências
estritamente descendentes. Detalhes em [docs/arquitetura.md](docs/arquitetura.md).

## Camadas

| Biblioteca | Pasta | Depende de | Conteúdo |
|---|---|---|---|
| `justcad::geom` | `src/geom` | nada | Point2, Vec2, BBox, kernel numérico (`DoubleKernel`) e, depois, curvas |
| `justcad::topo` | `src/topo` | nada | IDs fortes e estrutura half-edge |
| `justcad::euler` | `src/euler` | topo, geom | operadores de Euler de baixo e alto nível |
| `justcad::model` | `src/model` | euler, geom | API pública do modelo geométrico |
| app | `src/app` (M4) | model | UI com SDL3 + Dear ImGui |

## Compilar e testar

Requer CMake 3.25+ e um compilador C++23 (GCC 13, Clang 18 ou MSVC 19.38+).
O Catch2 é baixado automaticamente via FetchContent.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Roadmap

- **M0** esqueleto, CI, geometria básica e kernel numérico ← *atual*
- **M1** half-edge, operadores de Euler de baixo nível, `validate()`
- **M2** operadores de alto nível, undo/redo por transação
- **M3** modelo geométrico com retas e detecção automática de regiões
- **M4** UI mínima (SDL3 + Dear ImGui)
- **M5** Bézier de grau arbitrário, arcos, círculos, elipses
- **M6** persistência e import/export

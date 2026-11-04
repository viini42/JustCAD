# JustCAD

A 2D modeler for drawing geometric primitives (points, lines, polylines, Bézier
curves) and closed regions (rectangles, regular polygons, circles, ellipses and
regions bounded by primitives).

Internally the model is a **half-edge** topological data structure that is
modified only through **Euler operators**, organized in layers with strictly
downward dependencies. See [docs/architecture.md](docs/architecture.md) for the
design and [docs/style.md](docs/style.md) for naming and code rules.

## Layers

| Library | Folder | Depends on | Contents |
|---|---|---|---|
| `JustCad::Core` | `src/core` | — | `JUSTCAD_ASSERT` and shared utilities |
| `JustCad::Geom` | `src/geom` | Core | Point2, Vec2, BBox, numeric kernel (`DoubleKernel`), later curves |
| `JustCad::Topo` | `src/topo` | Core | strong IDs and the half-edge data structure |
| `JustCad::Euler` | `src/euler` | Topo, Geom | low- and high-level Euler operators |
| `JustCad::Model` | `src/model` | Euler, Geom | public API of the geometric model |
| App | `src/app` (M4) | Model | UI with SDL3 + Dear ImGui |

## Build and test

Requires CMake 3.25+ and a C++23 compiler (GCC 13, Clang 18 or MSVC 19.38+).
GoogleTest is fetched automatically through FetchContent.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Roadmap

- **M0** skeleton, CI, basic geometry and numeric kernel ← *current*
- **M1** half-edge structure, low-level Euler operators, `Validate()`
- **M2** high-level operators, transactional undo/redo
- **M3** geometric model with straight edges and automatic region detection
- **M4** minimal UI (SDL3 + Dear ImGui)
- **M5** arbitrary-degree Bézier, arcs, circles, ellipses
- **M6** persistence and import/export

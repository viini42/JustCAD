# JustCAD

A 2D modeler for drawing geometric primitives (points, lines, polylines, Bézier
curves) and closed regions (rectangles, regular polygons, circles, ellipses and
regions bounded by primitives).

Internally the model is a **half-edge** topological data structure that is
modified only through **Euler operators**, organized in layers with strictly
downward dependencies. See [docs/architecture.md](docs/architecture.md) for the
design, [docs/style.md](docs/style.md) for naming and code rules, and
[CLAUDE.md](CLAUDE.md) for the layout and Definition of Done. Code is formatted with the
repository's `.clang-format` (clang-format 18).

## Layers

Headers live in `include/jcad/<layer>/`, sources in `src/` and tests in `tests/` (both flat).

| Library | Headers | Depends on | Contents |
|---|---|---|---|
| `JustCad::Core` | `core/` | — | `JCAD_ASSERT` and shared utilities |
| `JustCad::Geom` | `geom/` | Core | Point2, Vec2, BBox, numeric kernel (`DoubleKernel`), later curves |
| `JustCad::Topo` | `topo/` | Core | strong IDs and the half-edge data structure |
| `JustCad::Euler` | `euler/` | Topo, Geom | low- and high-level Euler operators |
| `JustCad::Model` | `model/` | Euler, Geom | public API of the geometric model |
| App | `app/` (M4) | Model | UI with SDL3 + Dear ImGui |

## Build and test

Requires CMake 3.25+ and a C++23 compiler (GCC 13, Clang 18 with libc++, or MSVC 19.38+).
GoogleTest is fetched automatically through FetchContent.

```sh
cmake --workflow --preset dev       # Debug, warnings as errors, runs the tests
cmake --workflow --preset release   # same in Release
```

## Roadmap

- **M0** skeleton, CI, basic geometry and numeric kernel
- **M1** half-edge structure, low-level Euler operators, `Validate()`
- **M2** high-level operators, transactional undo/redo
- **M3** geometric model with straight edges and automatic region detection ← *current*
- **M4** minimal UI (SDL3 + Dear ImGui)
- **M5** arbitrary-degree Bézier, arcs, circles, ellipses
- **M6** persistence and import/export

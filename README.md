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
| `JustCad::App` | `app/` | Model | editor logic (tools, selection, snapping) and viewport, toolkit-free |
| `JustCad` (executable) | `src/main.cpp` | App | window and UI with SDL3 + Dear ImGui |

## Build and test

Requires CMake 3.25+ and a C++23 compiler (GCC 13, Clang 18 with libc++, or MSVC 19.38+).
GoogleTest, SDL3 and Dear ImGui are fetched automatically through FetchContent
(on Linux SDL3 needs the X11 or Wayland development packages, e.g. `libx11-dev libxext-dev`).
Configure with `-DJCAD_BUILD_GUI=OFF` to build only the libraries and tests.

```sh
cmake --workflow --preset dev       # Debug, warnings as errors, runs the tests
cmake --workflow --preset release   # same in Release
./build/dev/src/JustCad             # run the editor
```

## Using the editor

| Input | Action |
|---|---|
| `S` `P` `L` `O` `R` `G` | tools: select, point, line, polyline, rectangle, regular polygon |
| left click | place a point / pick (snaps to nearby vertices, then to the grid) |
| right click or `Enter` | finish an open polyline; click its first vertex to close it |
| `Esc` | cancel the shape in progress and the selection |
| `Delete` / `Backspace` | remove the selected vertex or edge |
| `Ctrl+Z`, `Ctrl+Y` / `Ctrl+Shift+Z` | undo, redo |
| middle or right drag, wheel | pan, zoom |

Regions closed by any mix of primitives are filled automatically.

## Roadmap

- **M0** skeleton, CI, basic geometry and numeric kernel
- **M1** half-edge structure, low-level Euler operators, `Validate()`
- **M2** high-level operators, transactional undo/redo
- **M3** geometric model with straight edges and automatic region detection
- **M4** minimal UI (SDL3 + Dear ImGui) ← *current*
- **M5** arbitrary-degree Bézier, arcs, circles, ellipses
- **M6** persistence and import/export

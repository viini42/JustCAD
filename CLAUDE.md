# JustCAD

2D modeler on a half-edge topology driven by Euler operators. Design: `docs/architecture.md`.
Naming and code rules: `docs/style.md` (read before writing code). New files: `.claude/skills/cpp-new-file`. Commits and branches: `.claude/skills/git-commit`.

All code, comments, docs, commit messages and PR text are in English.

## Layout

- `include/jcad/<area>/<name>.hpp` — headers; the include root is `include/jcad`, so includes
  read `#include "geom/vec2.hpp"`.
- `src/<name>.cpp` — flat; `src/CMakeLists.txt` lists each source under its layer's library.
- `tests/test_<name>.cpp` — flat, one per source file; `tests/CMakeLists.txt` has one executable per layer.
- Layers (libraries) depend only downwards: Core ← Geom, Topo ← Euler ← Model ← App.
  A header in a lower layer must never include one from a higher layer.

## Definition of Done

1. Configure, build (Debug, warnings as errors) and run all tests: `cmake --workflow --preset dev`.
   Use `--preset release` to also check the Release build (asserts compiled out).
2. Formatting is clean (clang-format 18):
   `clang-format --dry-run --Werror include/jcad/*/*.hpp src/*.cpp tests/*.cpp`
3. Every new source file has its `tests/test_<name>.cpp`.
4. Docs updated when a decision or public API changes.

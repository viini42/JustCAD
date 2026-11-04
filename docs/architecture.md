# JustCAD — Architecture

> Status: v1. Main decisions settled on 2026-11-03 (§11). Items marked **[assumption]** may still change.

## 1. Goal

A 2D modeler for drawing points, lines, polylines, Bézier curves and closed regions (rectangles, regular polygons, circles, ellipses, regions bounded by primitives). Internally everything is represented by a **half-edge** topological data structure that is modified exclusively through **Euler operators**, organized in layers with strictly downward dependencies.

## 2. Layer overview

```
 ┌──────────────────────────────────────────────┐
 │ 5. App / UI (window, input, render, commands) │
 ├──────────────────────────────────────────────┤
 │ 4. Geometric model (public API: AddLine,      │
 │    AddCircle, Split, Delete, queries, undo)   │
 ├──────────────────────────────────────────────┤
 │ 3. High-level Euler operators                 │
 │    (AddEdge, SplitEdge, RemoveEdge, ...)      │
 ├──────────────────────────────────────────────┤
 │ 2. Low-level Euler operators                  │
 │    (MVFS/KVFS, MEV/KEV, MEF/KEF, KEMR/MEKR)   │
 ├──────────────────────────────────────────────┤
 │ 1. Half-edge topological data structure       │
 └──────────────────────────────────────────────┘
        ▲ side library: Geometry (Curve, Point2, BBox) — no dependency on topology
```

Coupling rule: each layer only knows the layer directly below it (plus the geometry library). Topology does **not** know what a line or a Bézier is; it only stores an opaque handle to the edge's geometry.

## 3. Layer 1 — Half-edge data structure

Entities live in pools/arrays and refer to each other through stable integer IDs, not raw pointers. This makes undo, serialization and validation easier.

| Entity | Main fields |
|---|---|
| `Vertex` | `HalfEdgeId outgoing`, `GeomPointId point` |
| `HalfEdge` | `VertexId origin`, `HalfEdgeId twin, next, prev`, `LoopId loop`, `EdgeId edge` |
| `Edge` | `HalfEdgeId half_edges[2]`, `CurveId curve`, curve direction relative to `half_edges[0]` |
| `Loop` | `HalfEdgeId first`, `FaceId face`, `bool is_outer` |
| `Face` | `LoopId outer`, `std::vector<LoopId> inner` (holes), attributes |
| `Model` | sets of faces/vertices/edges; owns the unbounded face |

Decisions (settled 2026-11-03):
- **Explicit unbounded face**: the plane itself is a face and every primitive lives inside it. Dangling lines, isolated points and regions are all handled the same way.
- **Inner loops** (holes). Besides real holes, this is how every disconnected drawing is stored inside the unbounded face, so it is used all the time internally. Required by KEMR/MEKR.
- **Dangling ("hair") edges**: twin half-edges in the same loop, used by open lines and polylines that do not close a region.
- **Euler–Poincaré invariant** checked by a debug-mode `Validate()`. Counting the unbounded face in `F`, the formula is `V − E + F = 1 + H`, where `H` is the number of inner loops (equal to the number of connected components, since each component lives as exactly one inner loop of its enclosing face; an isolated vertex counts as a loop). Examples: isolated vertex 1−0+1 = 2 (H=1); triangle 3−3+2 = 2 (H=1); two separate lines 4−2+1 = 3 (H=2); a square inside another square without touching 8−8+3 = 3 (H=2). `Validate()` also checks local consistency (`twin(twin(h)) = h`, `next(prev(h)) = h`, every half-edge of a loop points back to it).

## 4. Layer 2 — Low-level Euler operators

A minimal and complete set (with inverses), in the style of Mäntylä:

| Operator | Effect | Inverse |
|---|---|---|
| `MVFS` | make vertex + face (first point; the face is the unbounded one) | `KVFS` |
| `MEV` | make an edge and a new vertex from an existing vertex | `KEV` |
| `MEF` | make an edge between two vertices of the same loop, splitting the face | `KEF` |
| `KEMR` | kill an edge, making a ring (inner loop) | `MEKR` |
| `MVR`/`KVR` (optional) | isolated vertex as a ring inside a face | — |

Rules:
- Each operator validates its preconditions, only changes IDs/links and **returns a record of its inverse**, the natural basis for undo/redo.
- No geometry is computed here; operators receive ready-made geometry IDs.

## 5. Layer 3 — High-level Euler operators

Compositions of low-level operators with modeling semantics, still purely topological (but may query geometry to decide which loop to insert into):

- `AddIsolatedVertex(p)`
- `AddEdge(v1, v2, curve)` — picks MEV, MEF or MEKR depending on whether v1/v2 are connected, in the same loop, or in different loops.
- `SplitEdge(e, t)` — inserts a vertex on an edge (splits the curve in two).
- `MergeEdges(v)` — removes a degree-2 vertex (inverse of `SplitEdge`).
- `RemoveEdge(e)` — picks KEV, KEF or KEMR.
- `RemoveVertex(v)`.
- Transactions: group several operators into a single undo entry.

## 6. Layer 4 — Geometric model (public API)

The functions the rest of the app uses; they translate primitives into topology:

| Primitive | Topological construction |
|---|---|
| Point | isolated vertex |
| Line | 2 vertices + 1 edge (`LineSegment`) |
| Open polyline | n vertices + n−1 line edges (see §7) |
| Bézier | 2 vertices + 1 edge (`Bezier`) |
| Rectangle / regular polygon | n vertices + n edges closing a face (`MEF` at the end) |
| Circle / ellipse | 1 vertex + 1 closed edge (self-loop) `Arc/Ellipse`, or 2 vertices + 2 arcs **[assumption]** |
| Region bounded by primitives | **planar arrangement**, detected automatically: when an edge is inserted, intersections with existing edges are computed, edges are split (`SplitEdge`) and the pieces inserted; faces appear automatically through `MEF` |

Extra responsibilities of this layer: curve×curve intersection, snapping/tolerance, queries (point in face, nearest edge, bbox), spatial index (brute force first, then grid/quadtree).

## 7. Generic edge abstraction (geometry)

Topology-independent interface:

```cpp
// Operations every curve kind provides (as a variant + std::visit, or virtual):
Point2 Eval(double t);                   // t in [0, 1]
Vec2   Tangent(double t);
BBox   Bounds();
double Length();
double ClosestParam(Point2 p);
std::pair<Curve, Curve> Split(double t);
Curve  Reversed();
void   Tessellate(double tol, std::vector<Point2>& out);
CurveKind Kind();
// LineSegment, Polyline, Bezier (arbitrary degree), CircularArc, EllipticArc
```

**Arbitrary-degree Bézier** (settled): `Bezier { std::vector<Point2> control_points; }` with degree `n = control_points.size() − 1`. Evaluation and `Split(t)` via de Casteljau; the derivative is a degree `n−1` Bézier; bounds from the convex hull of the control points (conservative), refined with the roots of the derivative. Bézier×anything intersection by recursive subdivision with bbox culling (or *Bézier clipping*), finished by Newton refinement. Degree elevation is available for operations that need matching degrees.

- Intersections dispatched through a `(kind, kind)` table to specialized functions (analytic line×line, the rest via Bézier subdivision/tessellation + Newton refinement).
- Alternative to virtual inheritance: `std::variant<LineSegment, Polyline, Bezier, Arc, ...>` + `std::visit` (no allocation, easier to serialize). **`variant` is recommended**; still open.
- **Polyline**: represented as *n line edges* (each inner vertex is a topological vertex), grouped by an "entity" attribute for selection/editing. This keeps topology honest (intersections at inner vertices need no special case). A `Polyline` curve kind remains available if a single edge is ever preferred.

### 7.1 Numeric kernel (double ↔ exact arithmetic)

Settled: `double` with a global tolerance for now, behind an abstraction that allows switching to exact arithmetic later. Inspired by CGAL kernels:

```cpp
template <class K>
concept kernel = requires {
    typename K::Scalar;                    // double, or an exact rational
    typename K::Point2;  typename K::Vec2;
} && requires (typename K::Point2 a, typename K::Point2 b, typename K::Point2 c) {
    { K::Orientation(a, b, c) } -> std::same_as<Sign>;  // left / right / collinear
    { K::Equal(a, b) }          -> std::same_as<bool>;  // uses the tolerance in DoubleKernel
    { K::CompareXY(a, b) }      -> std::same_as<Sign>;
};

class DoubleKernel { using Scalar = double; inline static double s_tolerance = 1e-9; ... };
// future: ExactKernel (rationals), FilteredKernel (double with exact fallback, Shewchuk-style)
```

Rules that keep the switch simple:
- **Every topological decision goes through kernel predicates** (`Orientation`, `Equal`, `CompareXY`, `OnSegment`). No `if (abs(x) < 1e-9)` scattered around the code.
- Constructions (intersection points, point at `t`) are kept apart from predicates, because in an exact kernel they can be expensive or not representable (Bézier intersections are not rational); for curves the exact kernel may fall back to intervals/filters.
- `geom/`, `euler/high/` and `model/` are templated on `K`. `topo/` and `euler/low/` do not depend on the kernel (IDs only).
- The global tolerance is configurable, and tests run the same suite under different tolerance values.

## 8. User entities vs. topology

A thin layer above the model: `Entity { id, kind (Rect, Circle, Polyline...), params, std::vector<EdgeId> }`. It lets the user edit "the rectangle" as an object even after intersections have split its topology. Each `Edge` stores the `EntityId`(s) it came from.

## 9. Code layout (C++23 + CMake)

```
JustCAD/
  CMakeLists.txt
  src/
    geom/      Point2, Vec2, BBox, kernel, Curve (variant), intersections
    topo/      half-edge data structure, ids/pools, Validate()
    euler/     low/ (MVFS, MEV, MEF, KEMR + inverses)  high/ (AddEdge, SplitEdge...)
    model/     GeometricModel (public API), entities, undo/redo
    app/       window, rendering, drawing tools   (depends on model/ only)
  tests/       one test file per source file, one executable per layer (GoogleTest)
```

Each folder is a separate CMake library with explicit dependencies, so the build itself rejects layer violations. Naming conventions are in [style.md](style.md).

## 10. Roadmap

1. **M0** — CMake skeleton, CI, basic `geom/` and numeric kernel with tests.
2. **M1** — half-edge + low-level operators + `Validate()`; tests for invariants and "operator followed by its inverse = identity".
3. **M2** — high-level operators + transactional undo/redo.
4. **M3** — geometric model with lines/polylines/rectangles/regular polygons (straight edges only) + line×line intersection → automatic regions.
5. **M4** — minimal UI (SDL3 + Dear ImGui): draw, select, delete, show filled faces.
6. **M5** — arbitrary-degree Bézier, arcs, circles, ellipses + curve intersections.
7. **M6** — persistence (own JSON format), import/export (SVG/DXF), advanced queries.

## 11. Decisions (2026-11-03)

1. Language/build: **C++23 + CMake**.
2. UI/rendering: **SDL3 + Dear ImGui**.
3. Regions bounded by primitives: **detected automatically** from intersections while drawing.
4. Bézier: **arbitrary degree** (§7).
5. Precision: **`double` with a global tolerance**, behind a kernel that can be swapped for exact arithmetic (§7.1).
6. All code, documentation and text in **English**; naming conventions in [style.md](style.md).

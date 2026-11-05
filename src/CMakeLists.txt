# One library per layer; dependencies only point downwards:
#   Core  : assertions and other utilities shared by every layer
#   Geom  : pure geometry + numeric kernel (knows nothing about topology)
#   Topo  : half-edge data structure (knows nothing about geometry)
#   Euler : low- and high-level Euler operators   -> Topo, Geom
#   Model : geometric model (public API)          -> Euler, Geom
#   App   : SDL3 + Dear ImGui UI                  -> Model        (milestone M4)
# Sources are listed explicitly; do not glob.
set(JCAD_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/jcad)

add_library(JustCadCore STATIC
    assert.cpp)
add_library(JustCad::Core ALIAS JustCadCore)
target_include_directories(JustCadCore PUBLIC ${JCAD_INCLUDE_DIR})
target_compile_features(JustCadCore PUBLIC cxx_std_23)
target_link_libraries(JustCadCore PRIVATE JustCad::CompilationOptions)

add_library(JustCadGeom STATIC
    double_kernel.cpp
    scoped_tolerance.cpp)
add_library(JustCad::Geom ALIAS JustCadGeom)
target_link_libraries(JustCadGeom
    PUBLIC JustCad::Core
    PRIVATE JustCad::CompilationOptions)

# Topology does not depend on Geom: geometry is referenced through opaque Ids.
add_library(JustCadTopo STATIC
    topology.cpp)
add_library(JustCad::Topo ALIAS JustCadTopo)
target_link_libraries(JustCadTopo
    PUBLIC JustCad::Core
    PRIVATE JustCad::CompilationOptions)

add_library(JustCadEuler STATIC
    euler_op.cpp
    high_level_ops.cpp
    history.cpp)
add_library(JustCad::Euler ALIAS JustCadEuler)
target_link_libraries(JustCadEuler
    PUBLIC JustCad::Topo JustCad::Geom
    PRIVATE JustCad::CompilationOptions)

add_library(JustCadModel STATIC
    geometric_model.cpp)
add_library(JustCad::Model ALIAS JustCadModel)
target_link_libraries(JustCadModel
    PUBLIC JustCad::Euler JustCad::Geom
    PRIVATE JustCad::CompilationOptions)

# Topology does not depend on Geom: it only stores opaque IDs to the geometry.
add_library(JustCadTopo INTERFACE)
add_library(JustCad::Topo ALIAS JustCadTopo)
target_include_directories(JustCadTopo INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(JustCadTopo INTERFACE JustCad::Core)

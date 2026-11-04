# Operadores de Euler: low/ (MVFS, MEV, MEF, KEMR e inversos) e high/ (addEdge,
# splitEdge, removeEdge...). Implementação a partir do marco M1.
add_library(justcad_euler INTERFACE)
add_library(justcad::euler ALIAS justcad_euler)
target_link_libraries(justcad_euler INTERFACE justcad::topo justcad::geom)

# Euler operators: low/ (MVFS, MEV, MEF, KEMR and inverses) and high/ (AddEdge,
# SplitEdge, RemoveEdge...). Implemented from milestone M1 on.
add_library(JustCadEuler INTERFACE)
add_library(JustCad::Euler ALIAS JustCadEuler)
target_link_libraries(JustCadEuler INTERFACE JustCad::Topo JustCad::Geom)

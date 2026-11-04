#include "justcad/geom/scoped_tolerance.hpp"

#include "justcad/geom/double_kernel.hpp"

justcad::geom::ScopedTolerance::ScopedTolerance(double tolerance)
    : m_previous{DoubleKernel::Tolerance()} {
    DoubleKernel::SetTolerance(tolerance);
}

justcad::geom::ScopedTolerance::~ScopedTolerance() {
    DoubleKernel::SetTolerance(m_previous);
}

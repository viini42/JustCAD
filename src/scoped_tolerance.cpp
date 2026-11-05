#include "geom/scoped_tolerance.hpp"

#include "geom/double_kernel.hpp"

jcad::geom::ScopedTolerance::ScopedTolerance(double tolerance) :
    m_previous{ DoubleKernel::Tolerance() }
{
  DoubleKernel::SetTolerance(tolerance);
}

jcad::geom::ScopedTolerance::~ScopedTolerance()
{
  DoubleKernel::SetTolerance(m_previous);
}

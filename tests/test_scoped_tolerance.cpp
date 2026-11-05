#include "geom/double_kernel.hpp"
#include "geom/scoped_tolerance.hpp"

#include <gtest/gtest.h>

using jcad::geom::DoubleKernel;
using jcad::geom::ScopedTolerance;

TEST(ScopedTolerance, ConstructorSetsKernelTolerance)
{
  const ScopedTolerance tolerance{ 0.5 };
  EXPECT_EQ(DoubleKernel::Tolerance(), 0.5);
}

TEST(ScopedTolerance, DestructorRestoresPreviousTolerance)
{
  const double before{ DoubleKernel::Tolerance() };
  {
    const ScopedTolerance tolerance{ 0.5 };
  }
  EXPECT_EQ(DoubleKernel::Tolerance(), before);
}

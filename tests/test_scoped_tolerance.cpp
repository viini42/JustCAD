#include "justcad/geom/scoped_tolerance.hpp"

#include <gtest/gtest.h>

#include "justcad/geom/double_kernel.hpp"

using justcad::geom::DoubleKernel;
using justcad::geom::ScopedTolerance;

TEST(ScopedTolerance, ConstructorSetsKernelTolerance) {
    const ScopedTolerance tolerance{0.5};
    EXPECT_EQ(DoubleKernel::Tolerance(), 0.5);
}

TEST(ScopedTolerance, DestructorRestoresPreviousTolerance) {
    const double before{DoubleKernel::Tolerance()};
    {
        const ScopedTolerance tolerance{0.5};
    }
    EXPECT_EQ(DoubleKernel::Tolerance(), before);
}

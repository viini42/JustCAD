#include "justcad/core/assert.hpp"

#include <gtest/gtest.h>

TEST(Assert, AssertTrueConditionDoesNothing) {
    JUSTCAD_ASSERT(1 + 1 == 2);
    JUSTCAD_ASSERT(2 > 1, "value {}", 2);
    SUCCEED();
}

TEST(AssertDeathTest, AssertFalseConditionAbortsWithFormattedMessage) {
    EXPECT_DEBUG_DEATH(JUSTCAD_ASSERT(1 > 2, "expected {} got {}", 1, 2),
                       "assertion failed: 1 > 2: expected 1 got 2");
}

TEST(AssertDeathTest, AssertConstantFalseAbortsWithMessage) {
    EXPECT_DEBUG_DEATH(JUSTCAD_ASSERT(false, "unreachable"), "assertion failed: false: unreachable");
}

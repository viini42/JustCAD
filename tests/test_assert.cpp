#include "core/assert.hpp"

#include <gtest/gtest.h>

TEST(Assert, AssertTrueConditionDoesNothing)
{
  JCAD_ASSERT(1 + 1 == 2);
  JCAD_ASSERT(2 > 1, "value {}", 2);
  SUCCEED();
}

TEST(AssertDeathTest, AssertFalseConditionAbortsWithFormattedMessage)
{
  EXPECT_DEBUG_DEATH(JCAD_ASSERT(1 > 2, "expected {} got {}", 1, 2),
                     "assertion failed: 1 > 2: expected 1 got 2");
}

TEST(AssertDeathTest, AssertConstantFalseAbortsWithMessage)
{
  EXPECT_DEBUG_DEATH(JCAD_ASSERT(false, "unreachable"), "assertion failed: false: unreachable");
}

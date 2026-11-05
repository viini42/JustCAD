#include "topo/id.hpp"
#include "topo/pool.hpp"

#include <gtest/gtest.h>

using jcad::topo::Pool;
using jcad::topo::VertexId;

TEST(Pool, AllocateReturnsIncreasingIds)
{
  Pool<VertexId, int> pool;
  EXPECT_EQ(pool.Allocate(10), VertexId{ 0 });
  EXPECT_EQ(pool.Allocate(20), VertexId{ 1 });
  EXPECT_EQ(pool.Size(), 2U);
  EXPECT_EQ(pool.Get(VertexId{ 1 }), 20);
}

TEST(Pool, AllocateAfterFreeReusesId)
{
  Pool<VertexId, int> pool;
  const VertexId first{ pool.Allocate(1) };
  static_cast<void>(pool.Allocate(2));
  pool.Free(first);
  EXPECT_FALSE(pool.IsAlive(first));
  EXPECT_EQ(pool.Allocate(3), first);
  EXPECT_EQ(pool.Get(first), 3);
}

TEST(Pool, AllocateAtPastEndGrowsAndKeepsGapsFree)
{
  Pool<VertexId, int> pool;
  pool.AllocateAt(VertexId{ 2 }, 7);
  EXPECT_EQ(pool.Size(), 1U);
  EXPECT_EQ(pool.Capacity(), 3U);
  EXPECT_FALSE(pool.IsAlive(VertexId{ 0 }));
  const VertexId reused{ pool.Allocate(8) };
  EXPECT_LT(reused.Value(), 2U);
}

TEST(Pool, AllocateAtFreedIdIsSkippedByLaterAllocate)
{
  Pool<VertexId, int> pool;
  const VertexId id{ pool.Allocate(1) };
  pool.Free(id);
  pool.AllocateAt(id, 2);
  EXPECT_EQ(pool.Allocate(3), VertexId{ 1 });
  EXPECT_EQ(pool.Get(id), 2);
}

TEST(Pool, AliveIdsListsOnlyAliveInOrder)
{
  Pool<VertexId, int> pool;
  static_cast<void>(pool.Allocate(1));
  const VertexId middle{ pool.Allocate(2) };
  static_cast<void>(pool.Allocate(3));
  pool.Free(middle);
  const std::vector<VertexId> expected{ VertexId{ 0 }, VertexId{ 2 } };
  EXPECT_EQ(pool.AliveIds(), expected);
}

TEST(Pool, IsAliveInvalidIdReturnsFalse)
{
  const Pool<VertexId, int> pool;
  EXPECT_FALSE(pool.IsAlive(VertexId{}));
  EXPECT_FALSE(pool.IsAlive(VertexId{ 5 }));
}

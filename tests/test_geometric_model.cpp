#include "geom/polygon.hpp"
#include "model/geometric_model.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <vector>

using jcad::model::GeometricModel;
using jcad::topo::FaceId;
using jcad::topo::Topology;
using Point2 = GeometricModel::Point2;

namespace
{

  class GeometricModelTest : public ::testing::Test
  {
  protected:
    void ExpectValid() const
    {
      const auto result{ m_model.GetTopology().Validate() };
      EXPECT_TRUE(result.has_value()) << result.error();
    }

    void ExpectCounts(std::size_t vertices, std::size_t edges, std::size_t boundedFaces) const
    {
      ExpectValid();
      const Topology& topology{ m_model.GetTopology() };
      EXPECT_EQ(topology.VertexCount(), vertices);
      EXPECT_EQ(topology.EdgeCount(), edges);
      EXPECT_EQ(topology.FaceCount(), boundedFaces + 1);
    }

    /** Every bounded face is counter-clockwise and the areas add up. */
    double TotalFaceArea() const
    {
      double total{ 0.0 };
      for (const GeometricModel::FaceOutline& face : m_model.BoundedFaces())
      {
        EXPECT_GT(jcad::geom::SignedArea(face.outer), 0.0);
        total += jcad::geom::SignedArea(face.outer);
        for (const std::vector<Point2>& hole : face.holes)
          total += jcad::geom::SignedArea(hole);
      }
      return total;
    }

    GeometricModel m_model;
  };

} // namespace

TEST_F(GeometricModelTest, AddPointMakesIsolatedVertex)
{
  m_model.AddPoint(Point2{ 1.0, 2.0 });
  ExpectCounts(1, 0, 0);
}

TEST_F(GeometricModelTest, AddPointTwiceReusesVertex)
{
  const auto first{ m_model.AddPoint(Point2{ 1.0, 2.0 }) };
  EXPECT_EQ(m_model.AddPoint(Point2{ 1.0, 2.0 }), first);
  ExpectCounts(1, 0, 0);
}

TEST_F(GeometricModelTest, AddLineMakesOpenSegment)
{
  m_model.AddLine(Point2{ 0.0, 0.0 }, Point2{ 1.0, 0.0 });
  ExpectCounts(2, 1, 0);
}

TEST_F(GeometricModelTest, AddRectangleCreatesBoundedFace)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 1.0 });
  ExpectCounts(4, 4, 1);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 2.0);
  EXPECT_NE(m_model.LocateFace(Point2{ 1.0, 0.5 }), m_model.GetTopology().UnboundedFace());
  EXPECT_EQ(m_model.LocateFace(Point2{ 3.0, 0.5 }), m_model.GetTopology().UnboundedFace());
}

TEST_F(GeometricModelTest, AddRectangleClockwiseStillMakesCounterClockwiseFace)
{
  m_model.AddPolyline({ { 0.0, 0.0 }, { 0.0, 1.0 }, { 1.0, 1.0 }, { 1.0, 0.0 } }, true);
  ExpectCounts(4, 4, 1);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 1.0);
}

TEST_F(GeometricModelTest, AddRegularPolygonCreatesFace)
{
  m_model.AddRegularPolygon(GeometricModel::RegularPolygon{ { 0.0, 0.0 }, 1.0, 6, 0.0 });
  ExpectCounts(6, 6, 1);
}

TEST_F(GeometricModelTest, AddLineAcrossRectangleSplitsFace)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 1.0, -1.0 }, Point2{ 1.0, 3.0 });
  ExpectCounts(8, 9, 2);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 4.0);
}

TEST_F(GeometricModelTest, CrossingLinesSplitAtIntersection)
{
  m_model.AddLine(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 0.0, 2.0 }, Point2{ 2.0, 0.0 });
  ExpectCounts(5, 4, 0);
}

TEST_F(GeometricModelTest, OverlappingRectanglesMakeThreeRegions)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddRectangle(Point2{ 1.0, 1.0 }, Point2{ 3.0, 3.0 });
  ExpectCounts(10, 12, 3);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 7.0);
}

TEST_F(GeometricModelTest, TriangleFromThreeSeparateLinesClosesRegion)
{
  m_model.AddLine(Point2{ 0.0, 0.0 }, Point2{ 4.0, 0.0 });
  m_model.AddLine(Point2{ 4.0, 0.0 }, Point2{ 0.0, 3.0 });
  ExpectCounts(3, 2, 0);
  m_model.AddLine(Point2{ 0.0, 3.0 }, Point2{ 0.0, 0.0 });
  ExpectCounts(3, 3, 1);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 6.0);
}

TEST_F(GeometricModelTest, NestedRectangleBecomesHole)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 4.0, 4.0 });
  m_model.AddRectangle(Point2{ 1.0, 1.0 }, Point2{ 2.0, 2.0 });
  ExpectCounts(8, 8, 2);
  const FaceId outer{ m_model.LocateFace(Point2{ 3.0, 3.0 }) };
  EXPECT_EQ(m_model.GetTopology().FaceAt(outer).inner.size(), 1U);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 16.0);
}

TEST_F(GeometricModelTest, RectangleDrawnAroundExistingShapeAdoptsIt)
{
  m_model.AddRectangle(Point2{ 1.0, 1.0 }, Point2{ 2.0, 2.0 });
  m_model.AddPoint(Point2{ 3.0, 3.0 });
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 4.0, 4.0 });
  ExpectCounts(9, 8, 2);
  const FaceId outer{ m_model.LocateFace(Point2{ 3.5, 0.5 }) };
  EXPECT_EQ(m_model.GetTopology().FaceAt(outer).inner.size(), 2U);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 16.0);
}

TEST_F(GeometricModelTest, PointInsideFaceLandsInThatFace)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddPoint(Point2{ 1.0, 1.0 });
  ExpectValid();
  const FaceId face{ m_model.LocateFace(Point2{ 0.5, 0.5 }) };
  EXPECT_EQ(m_model.GetTopology().FaceAt(face).inner.size(), 1U);
}

TEST_F(GeometricModelTest, LineAlongExistingEdgeDoesNotDuplicateIt)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ -1.0, 0.0 }, Point2{ 1.0, 0.0 });
  ExpectCounts(6, 6, 1);
}

TEST_F(GeometricModelTest, LineThroughVertexUsesIt)
{
  m_model.AddPoint(Point2{ 1.0, 0.0 });
  m_model.AddLine(Point2{ 0.0, 0.0 }, Point2{ 2.0, 0.0 });
  ExpectCounts(3, 2, 0);
}

TEST_F(GeometricModelTest, DiagonalsOfSquareMakeFourTriangles)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 0.0, 2.0 }, Point2{ 2.0, 0.0 });
  ExpectCounts(5, 8, 4);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 4.0);
}

TEST_F(GeometricModelTest, RemoveEdgeMergesFacesAndCleansVertices)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 1.0, 0.0 }, Point2{ 1.0, 2.0 });
  ExpectCounts(6, 7, 2);
  for (const GeometricModel::EdgeSegment& segment : m_model.EdgeSegments())
  {
    if (segment.start.x == 1.0 && segment.end.x == 1.0)
      m_model.RemoveEdge(segment.edge);
  }
  ExpectCounts(4, 4, 1);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 4.0);
}

TEST_F(GeometricModelTest, RemoveEdgeOfBridgeKeepsOuterBoundary)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 4.0, 4.0 });
  m_model.AddRectangle(Point2{ 1.0, 1.0 }, Point2{ 2.0, 2.0 });
  m_model.AddLine(Point2{ 0.0, 1.0 }, Point2{ 1.0, 1.0 });
  ExpectValid();
  for (const GeometricModel::EdgeSegment& segment : m_model.EdgeSegments())
  {
    if (segment.start.y == 1.0 && segment.end.y == 1.0 &&
        std::max(segment.start.x, segment.end.x) == 1.0)
      m_model.RemoveEdge(segment.edge);
  }
  ExpectCounts(8, 8, 2);
  EXPECT_DOUBLE_EQ(TotalFaceArea(), 16.0);
}

TEST_F(GeometricModelTest, RemoveVertexOfTriangleLeavesNothingDangling)
{
  m_model.AddPolyline({ { 0.0, 0.0 }, { 2.0, 0.0 }, { 0.0, 2.0 } }, true);
  m_model.RemoveVertex(m_model.AddPoint(Point2{ 2.0, 0.0 }));
  ExpectCounts(2, 1, 0);
}

TEST_F(GeometricModelTest, UndoRedoWholePrimitives)
{
  m_model.AddRectangle(Point2{ 0.0, 0.0 }, Point2{ 2.0, 2.0 });
  m_model.AddRectangle(Point2{ 1.0, 1.0 }, Point2{ 3.0, 3.0 });
  m_model.Undo();
  ExpectCounts(4, 4, 1);
  m_model.Undo();
  ExpectCounts(0, 0, 0);
  EXPECT_FALSE(m_model.CanUndo());
  m_model.Redo();
  m_model.Redo();
  ExpectCounts(10, 12, 3);
  EXPECT_FALSE(m_model.CanRedo());
}

namespace
{

  bool IsInsideItsFace(const GeometricModel& model, jcad::topo::LoopId loop)
  {
    const Topology& topology{ model.GetTopology() };
    const jcad::topo::Face& face{ topology.FaceAt(topology.LoopAt(loop).face) };
    if (!face.outer.Valid() || face.outer == loop || topology.LoopAt(loop).isolated_vertex.Valid())
      return true;
    const Point2 sample{ model.LoopPolygon(loop).front() };
    return jcad::geom::LocatePoint<GeometricModel::Kernel>(model.LoopPolygon(face.outer), sample) ==
           jcad::geom::PointLocation::INSIDE;
  }

  /** Valid topology, counter-clockwise outer loops and clockwise (or flat) inner loops. */
  ::testing::AssertionResult HasValidOrientedFaces(const GeometricModel& model)
  {
    const Topology& topology{ model.GetTopology() };
    const auto result{ topology.Validate() };
    if (!result)
      return ::testing::AssertionFailure() << result.error();
    for (const jcad::topo::LoopId loop : topology.LoopIds())
    {
      const double area{ model.LoopSignedArea(loop) };
      if (topology.IsOuterLoop(loop) ? area <= 0.0 : area > 0.0)
        return ::testing::AssertionFailure() << "loop " << loop.Value() << " has area " << area;
      if (!IsInsideItsFace(model, loop))
        return ::testing::AssertionFailure() << "loop " << loop.Value() << " lies outside its face";
    }
    return ::testing::AssertionSuccess();
  }

} // namespace

TEST(GeometricModel, AddRandomPrimitivesOnGridKeepsValidOrientedFaces)
{
  for (unsigned seed{ 1 }; seed <= 30; ++seed)
  {
    std::mt19937 random{ seed };
    std::uniform_int_distribution<int> coordinate{ 0, 6 };
    const auto random_point{ [&]
                             {
                               return Point2{ static_cast<double>(coordinate(random)),
                                              static_cast<double>(coordinate(random)) };
                             } };
    GeometricModel model;
    for (int step{ 0 }; step < 25; ++step)
    {
      const std::vector<GeometricModel::EdgeSegment> edges{ model.EdgeSegments() };
      if (step % 5 == 4 && !edges.empty())
        model.RemoveEdge(edges[static_cast<std::size_t>(coordinate(random)) % edges.size()].edge);
      else if (step % 3 == 0)
        model.AddRectangle(random_point(), random_point());
      else
        model.AddLine(random_point(), random_point());
      ASSERT_TRUE(HasValidOrientedFaces(model)) << "seed " << seed << " step " << step;
    }
    while (model.CanUndo())
      model.Undo();
    EXPECT_EQ(model.GetTopology(), Topology{}) << "seed " << seed;
  }
}

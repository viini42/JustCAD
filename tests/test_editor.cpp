#include "app/editor.hpp"
#include "topo/topology.hpp"

#include <gtest/gtest.h>

using jcad::app::Editor;
using jcad::app::Tool;
using Point2 = Editor::Point2;

namespace
{

  class EditorTest : public ::testing::Test
  {
  protected:
    std::size_t Vertices() const { return m_editor.Model().GetTopology().VertexCount(); }
    std::size_t Edges() const { return m_editor.Model().GetTopology().EdgeCount(); }
    std::size_t Faces() const { return m_editor.Model().GetTopology().FaceCount(); }

    Editor m_editor;
  };

} // namespace

TEST_F(EditorTest, PointToolAddsPointPerClick)
{
  m_editor.SetTool(Tool::POINT);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 1.0, 0.0 });
  EXPECT_EQ(Vertices(), 2U);
}

TEST_F(EditorTest, LineToolNeedsTwoClicks)
{
  m_editor.SetTool(Tool::LINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  EXPECT_EQ(Edges(), 0U);
  EXPECT_EQ(m_editor.PendingPoints().size(), 1U);
  m_editor.Click(Point2{ 2.0, 0.0 });
  EXPECT_EQ(Edges(), 1U);
  EXPECT_TRUE(m_editor.PendingPoints().empty());
}

TEST_F(EditorTest, RectangleToolMakesFace)
{
  m_editor.SetTool(Tool::RECTANGLE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Hover(Point2{ 2.0, 1.0 });
  EXPECT_EQ(m_editor.Preview().points.size(), 4U);
  EXPECT_TRUE(m_editor.Preview().closed);
  m_editor.Click(Point2{ 2.0, 1.0 });
  EXPECT_EQ(Faces(), 2U);
}

TEST_F(EditorTest, PolygonToolUsesConfiguredSides)
{
  m_editor.SetTool(Tool::POLYGON);
  m_editor.SetPolygonSides(5);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 1.0, 0.0 });
  EXPECT_EQ(Vertices(), 5U);
  EXPECT_EQ(Faces(), 2U);
}

TEST_F(EditorTest, PolylineClosesWhenClickingFirstVertex)
{
  m_editor.SetTool(Tool::POLYLINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 2.0, 0.0 });
  m_editor.Click(Point2{ 1.0, 2.0 });
  m_editor.Click(Point2{ 0.0, 0.0 });
  EXPECT_EQ(Edges(), 3U);
  EXPECT_EQ(Faces(), 2U);
}

TEST_F(EditorTest, PolylineFinishKeepsItOpen)
{
  m_editor.SetTool(Tool::POLYLINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 2.0, 0.0 });
  m_editor.Click(Point2{ 1.0, 2.0 });
  m_editor.Finish();
  EXPECT_EQ(Edges(), 2U);
  EXPECT_EQ(Faces(), 1U);
}

TEST_F(EditorTest, SnapPrefersNearbyVertexThenGrid)
{
  m_editor.SetTool(Tool::POINT);
  m_editor.Click(Point2{ 0.33, 0.33 });
  m_editor.SetPickRadius(0.1);
  m_editor.SetGridStep(0.5);
  EXPECT_EQ(m_editor.Snap(Point2{ 0.36, 0.30 }), (Point2{ 0.33, 0.33 }));
  EXPECT_EQ(m_editor.Snap(Point2{ 0.9, 1.2 }), (Point2{ 1.0, 1.0 }));
}

TEST_F(EditorTest, SelectAndDeleteEdge)
{
  m_editor.SetTool(Tool::LINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 2.0, 0.0 });
  m_editor.SetTool(Tool::SELECT);
  m_editor.Click(Point2{ 1.0, 0.05 });
  ASSERT_TRUE(m_editor.GetSelection().edge.Valid());
  m_editor.DeleteSelection();
  EXPECT_EQ(Edges(), 0U);
  EXPECT_EQ(Vertices(), 0U);
}

TEST_F(EditorTest, SelectVertexWinsOverEdge)
{
  m_editor.SetTool(Tool::LINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 2.0, 0.0 });
  m_editor.SetTool(Tool::SELECT);
  m_editor.Click(Point2{ 0.02, 0.0 });
  EXPECT_TRUE(m_editor.GetSelection().vertex.Valid());
  EXPECT_FALSE(m_editor.GetSelection().edge.Valid());
}

TEST_F(EditorTest, UndoDropsPendingShapeBeforeModelChanges)
{
  m_editor.SetTool(Tool::LINE);
  m_editor.Click(Point2{ 0.0, 0.0 });
  m_editor.Click(Point2{ 2.0, 0.0 });
  m_editor.Click(Point2{ 5.0, 5.0 });
  m_editor.Undo();
  EXPECT_TRUE(m_editor.PendingPoints().empty());
  EXPECT_EQ(Edges(), 1U);
  m_editor.Undo();
  EXPECT_EQ(Edges(), 0U);
  m_editor.Redo();
  EXPECT_EQ(Edges(), 1U);
}

#include "app/editor.hpp"
#include "app/tool.hpp"
#include "app/viewport.hpp"
#include "geom/polygon.hpp"
#include "geom/triangulate.hpp"
#include "topo/topology.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <memory>
#include <vector>

namespace
{

  using jcad::app::Editor;
  using jcad::app::Tool;
  using jcad::app::Viewport;
  using Point2 = Editor::Point2;

  constexpr float PICK_RADIUS_PX{ 6.0F };
  constexpr ImU32 EDGE_COLOR{ IM_COL32(230, 230, 230, 255) };
  constexpr ImU32 SELECTED_COLOR{ IM_COL32(255, 170, 40, 255) };
  constexpr ImU32 PREVIEW_COLOR{ IM_COL32(90, 200, 255, 255) };
  constexpr ImU32 GRID_COLOR{ IM_COL32(255, 255, 255, 18) };

  struct ToolButton
  {
    Tool tool;
    const char* label;
    ImGuiKey key;
  };

  constexpr std::array<ToolButton, 10> TOOLS{ {
    { Tool::SELECT, "Select (S)", ImGuiKey_S },
    { Tool::POINT, "Point (P)", ImGuiKey_P },
    { Tool::LINE, "Line (L)", ImGuiKey_L },
    { Tool::POLYLINE, "Polyline (O)", ImGuiKey_O },
    { Tool::RECTANGLE, "Rectangle (R)", ImGuiKey_R },
    { Tool::POLYGON, "Polygon (G)", ImGuiKey_G },
    { Tool::CIRCLE, "Circle (C)", ImGuiKey_C },
    { Tool::ELLIPSE, "Ellipse (E)", ImGuiKey_E },
    { Tool::ARC, "Arc, 3 points (A)", ImGuiKey_A },
    { Tool::BEZIER, "Bezier (B)", ImGuiKey_B },
  } };

  struct SdlWindowDeleter
  {
    void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
  };

  struct SdlRendererDeleter
  {
    void operator()(SDL_Renderer* renderer) const { SDL_DestroyRenderer(renderer); }
  };

  ImVec2 ToScreen(const Viewport& viewport, Point2 world)
  {
    const Point2 screen{ viewport.WorldToScreen(world) };
    return ImVec2{ static_cast<float>(screen.x), static_cast<float>(screen.y) };
  }

  Point2 FromScreen(const Viewport& viewport, ImVec2 screen)
  {
    return viewport.ScreenToWorld(Point2{ screen.x, screen.y });
  }

  ImU32 FaceColor(std::uint32_t face)
  {
    // Golden-ratio hues keep neighbouring faces apart.
    const float hue{ std::fmod(static_cast<float>(face) * 0.618034F, 1.0F) };
    float r{};
    float g{};
    float b{};
    ImGui::ColorConvertHSVtoRGB(hue, 0.45F, 0.65F, r, g, b);
    return ImGui::GetColorU32(ImVec4{ r, g, b, 0.55F });
  }

  std::vector<ImVec2> ToScreen(const Viewport& viewport, const std::vector<Point2>& points)
  {
    std::vector<ImVec2> screen;
    screen.reserve(points.size());
    for (const Point2& point : points)
      screen.push_back(ToScreen(viewport, point));
    return screen;
  }

  void DrawGrid(ImDrawList& draw, const Viewport& viewport, double step)
  {
    if (step <= 0.0 || step * viewport.Scale() < 8.0)
      return;
    const Viewport::Vec2 size{ viewport.ScreenSize() };
    const Point2 low{ viewport.ScreenToWorld(Point2{ 0.0, size.y }) };
    const Point2 high{ viewport.ScreenToWorld(Point2{ size.x, 0.0 }) };
    const auto width{ static_cast<float>(size.x) };
    const auto height{ static_cast<float>(size.y) };
    for (double x{ std::floor(low.x / step) * step }; x <= high.x; x += step)
    {
      const float sx{ ToScreen(viewport, Point2{ x, 0.0 }).x };
      draw.AddLine(ImVec2{ sx, 0.0F }, ImVec2{ sx, height }, GRID_COLOR);
    }
    for (double y{ std::floor(low.y / step) * step }; y <= high.y; y += step)
    {
      const float sy{ ToScreen(viewport, Point2{ 0.0, y }).y };
      draw.AddLine(ImVec2{ 0.0F, sy }, ImVec2{ width, sy }, GRID_COLOR);
    }
  }

  void DrawFaces(ImDrawList& draw, const Viewport& viewport, const Editor& editor)
  {
    // Painter's algorithm: larger faces first, so faces nested in holes paint over
    // their parent and no polygon-with-holes triangulation is needed.
    std::vector<jcad::model::GeometricModel::FaceOutline> faces{ editor.Model().BoundedFaces() };
    // Anti-aliased fringes would show the seams between triangles.
    const ImDrawListFlags flags{ draw.Flags };
    draw.Flags &= ~ImDrawListFlags_AntiAliasedFill;
    std::ranges::sort(faces,
                      [](const auto& a, const auto& b) {
                        return jcad::geom::SignedArea(a.outer) > jcad::geom::SignedArea(b.outer);
                      });
    for (const auto& face : faces)
    {
      const std::vector<ImVec2> outline{ ToScreen(viewport, face.outer) };
      const ImU32 color{ FaceColor(face.face.Value()) };
      for (const jcad::geom::Triangle& triangle :
           jcad::geom::Triangulate<Editor::Kernel>(face.outer))
        draw.AddTriangleFilled(outline[triangle[0]],
                               outline[triangle[1]],
                               outline[triangle[2]],
                               color);
    }
    draw.Flags = flags;
  }

  void DrawModel(ImDrawList& draw, const Viewport& viewport, const Editor& editor)
  {
    const Editor::Selection& selection{ editor.GetSelection() };
    // Half a pixel of chord error is invisible.
    const double tolerance{ 0.5 / viewport.Scale() };
    for (const auto& edge : editor.Model().EdgeCurves())
    {
      const bool selected{ edge.edge == selection.edge };
      const std::vector<ImVec2> points{ ToScreen(viewport,
                                                 jcad::geom::Tessellate(edge.curve, tolerance)) };
      draw.AddPolyline(points.data(),
                       static_cast<int>(points.size()),
                       selected ? SELECTED_COLOR : EDGE_COLOR,
                       ImDrawFlags_None,
                       selected ? 3.0F : 1.5F);
    }
    const jcad::topo::Topology& topology{ editor.Model().GetTopology() };
    for (const jcad::topo::VertexId vertex : topology.VertexIds())
    {
      const bool selected{ vertex == selection.vertex };
      draw.AddCircleFilled(ToScreen(viewport, editor.Model().PointOf(vertex)),
                           selected ? 5.0F : 3.0F,
                           selected ? SELECTED_COLOR : EDGE_COLOR);
    }
  }

  void DrawPreview(ImDrawList& draw, const Viewport& viewport, const Editor& editor)
  {
    const Editor::PreviewShape preview{ editor.Preview() };
    const std::vector<ImVec2> points{ ToScreen(viewport, preview.points) };
    if (points.size() >= 2)
    {
      draw.AddPolyline(points.data(),
                       static_cast<int>(points.size()),
                       PREVIEW_COLOR,
                       preview.closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                       1.5F);
    }
    for (const Point2& point : editor.PendingPoints())
      draw.AddCircleFilled(ToScreen(viewport, point), 4.0F, PREVIEW_COLOR);
  }

  void HandleShortcuts(Editor& editor)
  {
    const ImGuiIO& io{ ImGui::GetIO() };
    if (io.WantCaptureKeyboard)
      return;
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
      io.KeyShift ? editor.Redo() : editor.Undo();
    else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
      editor.Redo();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
      editor.Cancel();
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
      editor.Finish();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
      editor.DeleteSelection();
    if (io.KeyCtrl)
      return;
    for (const ToolButton& button : TOOLS)
    {
      if (ImGui::IsKeyPressed(button.key))
        editor.SetTool(button.tool);
    }
  }

  bool WasDragged(ImGuiMouseButton button)
  {
    const ImGuiIO& io{ ImGui::GetIO() };
    return io.MouseDragMaxDistanceSqr[button] >= io.MouseDragThreshold * io.MouseDragThreshold;
  }

  void HandleCanvasMouse(Editor& editor, Viewport& viewport)
  {
    const ImGuiIO& io{ ImGui::GetIO() };
    if (io.WantCaptureMouse)
      return;
    const Point2 world{ FromScreen(viewport, io.MousePos) };
    editor.SetPickRadius(PICK_RADIUS_PX / viewport.Scale());
    editor.Hover(world);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      editor.Click(world);
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !WasDragged(ImGuiMouseButton_Right))
      editor.Finish();
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Right))
      viewport.Pan(Viewport::Vec2{ io.MouseDelta.x, io.MouseDelta.y });
    if (io.MouseWheel != 0.0F)
      viewport.ZoomAt(Point2{ io.MousePos.x, io.MousePos.y },
                      std::pow(1.15, static_cast<double>(io.MouseWheel)));
  }

  void DrawToolbar(Editor& editor)
  {
    ImGui::SetNextWindowPos(ImVec2{ 10.0F, 10.0F }, ImGuiCond_FirstUseEver);
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    for (const ToolButton& button : TOOLS)
    {
      if (ImGui::RadioButton(button.label, editor.CurrentTool() == button.tool))
        editor.SetTool(button.tool);
    }
    int sides{ editor.PolygonSides() };
    if (ImGui::InputInt("Sides", &sides))
      editor.SetPolygonSides(sides);
    double grid{ editor.GridStep() };
    if (ImGui::InputDouble("Grid", &grid, 0.25, 1.0, "%.2f"))
      editor.SetGridStep(grid);
    ImGui::Separator();
    if (ImGui::Button("Undo"))
      editor.Undo();
    ImGui::SameLine();
    if (ImGui::Button("Redo"))
      editor.Redo();
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
      editor.DeleteSelection();
    const jcad::topo::Topology& topology{ editor.Model().GetTopology() };
    ImGui::Text("V %zu  E %zu  F %zu",
                topology.VertexCount(),
                topology.EdgeCount(),
                topology.FaceCount() - 1);
    ImGui::TextDisabled(
      "Right click / Enter: finish polyline or Bezier\nMiddle or right drag: pan, wheel: zoom");
    ImGui::End();
  }

  void Frame(Editor& editor, Viewport& viewport)
  {
    const ImGuiIO& io{ ImGui::GetIO() };
    viewport.SetScreenSize(Viewport::Vec2{ io.DisplaySize.x, io.DisplaySize.y });
    HandleShortcuts(editor);
    HandleCanvasMouse(editor, viewport);
    DrawToolbar(editor);
    ImDrawList& draw{ *ImGui::GetBackgroundDrawList() };
    DrawGrid(draw, viewport, editor.GridStep());
    DrawFaces(draw, viewport, editor);
    DrawModel(draw, viewport, editor);
    DrawPreview(draw, viewport, editor);
  }

  bool PollEvents()
  {
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        return false;
    }
    return true;
  }

  int RunWindow()
  {
    const std::unique_ptr<SDL_Window, SdlWindowDeleter> window{
      SDL_CreateWindow("JustCAD", 1280, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY)
    };
    const std::unique_ptr<SDL_Renderer, SdlRendererDeleter> renderer{
      window ? SDL_CreateRenderer(window.get(), nullptr) : nullptr
    };
    if (!renderer)
    {
      std::fprintf(stderr, "cannot create a window: %s\n", SDL_GetError());
      return 1;
    }
    SDL_SetRenderVSync(renderer.get(), 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
    ImGui_ImplSDLRenderer3_Init(renderer.get());

    Editor editor;
    Viewport viewport;
    editor.SetGridStep(0.5);
    while (PollEvents())
    {
      ImGui_ImplSDLRenderer3_NewFrame();
      ImGui_ImplSDL3_NewFrame();
      ImGui::NewFrame();
      Frame(editor, viewport);
      ImGui::Render();
      const ImGuiIO& io{ ImGui::GetIO() };
      SDL_SetRenderScale(renderer.get(),
                         io.DisplayFramebufferScale.x,
                         io.DisplayFramebufferScale.y);
      SDL_SetRenderDrawColor(renderer.get(), 28, 30, 34, 255);
      SDL_RenderClear(renderer.get());
      ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
      SDL_RenderPresent(renderer.get());
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return 0;
  }

  int Run()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      return 1;
    }
    // The window and renderer are destroyed inside RunWindow(), before SDL_Quit().
    const int status{ RunWindow() };
    SDL_Quit();
    return status;
  }

} // namespace

int main(int /*argc*/, char* /*argv*/[])
{
  try
  {
    return Run();
  }
  catch (const std::exception& error)
  {
    std::fprintf(stderr, "fatal: %s\n", error.what());
    return 1;
  }
}

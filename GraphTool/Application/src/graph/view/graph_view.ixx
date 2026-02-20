module;
#include <pch.h>

export module graph_view;

import graph_model;
import graph_view_model;

import quadtree;

struct GraphTheme {
    ImU32 m_backgroundColor{IM_COL32(20, 20, 20, 255)};
    ImU32 m_gridColor{IM_COL32(35, 35, 35, 255)};
    ImU32 m_minMaxColor{IM_COL32(255, 0, 0, 255)};
    ImU32 m_quadTreeColor{IM_COL32(0, 255, 255, 255)};

    ImU32 m_nodeColor{m_backgroundColor};
    ImU32 m_nodeBorderColor{IM_COL32(255, 255, 255, 255)};
    ImU32 m_selectedNodeBorderColor{IM_COL32(89, 222, 18, 255)};
    ImU32 m_hoveredNodeBorderColor{IM_COL32(18, 191, 222, 255)};
    ImU32 m_hoveredAndSelectedNodeBorderColor{IM_COL32(18, 222, 130, 255)};
};

export class GraphView {
   public:
    void initialize(const GraphModel* model, GraphViewModel* viewModel);

    void renderUI();
    void renderScene();

   private:
    void initializeGL();

    GLuint compileShader(GLenum type, const char* source);

    void drawMenuBar();
    void drawStatusBar();

    void drawNodesIndexes(ImDrawList* drawList);
    void drawQuadTree(ImDrawList* drawList, const QuadTree* quadTree);
    void drawMinMax(ImDrawList* drawList);

    void drawBackground();
    void drawNodes();

    ImU32 getNodeColor(const Node* node) const;

    bool shouldDrawNodes() const;

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};

    GraphTheme m_theme;

    bool m_drawGrid{true};
    bool m_drawQuadTree{false};
    bool m_drawMinMax{false};
    bool m_drawNodes{true};

    GLuint m_nodeTexture{};
    GLuint m_nodeOutlineTexture{};

    GLuint m_shaderProgram{};
    GLuint m_VAO{}, m_VBO{}, m_EBO{};
    GLuint m_quadVBO{}, m_instanceVBO{};
};

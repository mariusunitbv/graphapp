module;
#include <pch.h>

export module graph_view;

export import graph_view_model;
export import graph_model;

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
    void initialize();

    void renderUI();
    void renderScene();

    void onEvent(const SDL_Event& event);

    GraphViewModel* getViewModel() const;
    void setViewModel(GraphViewModel* viewModel);

    GraphModel* getModel() const;
    void setModel(GraphModel* model);

    GraphTheme& getTheme();
    void setTheme(const GraphTheme& theme);

    ImU32 getBackgroundColor() const;

    static ImVec4 getNodeWorldBBox(const Node* node);
    static ImVec4 getNodeWorldBBox(const ImVec2& worldPos);

    void buildQuadTree();

    static constexpr auto NODE_RADIUS = 28.f;

   private:
    void initializeShaders();

    GLuint compileShader(GLenum type, const char* source);

    const Node* getNode(NodeIndex_t nodeIndex) const;
    const Node* getNode(const ImVec2& screenPos) const;

    ImU32 getNodeColor(const Node* node) const;

    ImVec2 worldToScreen(const ImVec2& worldPos) const;
    ImVec2 screenToWorld(const ImVec2& screenPos) const;
    ImVec4 screenAreaToWorld(const ImVec2& screenPadding = ImVec2{}) const;

    void drawMenuBar();
    void drawStatusBar();

    void drawGrid(ImDrawList* drawList);
    void drawQuadTree(ImDrawList* drawList, QuadTree* quadTree);
    void drawMinMax(ImDrawList* drawList);
    void drawNodesText(ImDrawList* drawList);
    void drawNodesGL();
    void drawDragSelecting(ImDrawList* drawList);
    void drawMousePosition(ImDrawList* drawList);
    void drawUnfocusedBackground(ImDrawList* drawList);

    void onNodeAdded(const Node* node);
    void onLeftClick(float screenX, float screenY);
    void markNodeHovered(float screenX, float screenY);

    void queryVisible();
    void clearVisible();

    bool updateMinMaxBounds(const ImVec4& bbox);
    bool shouldDrawNodes() const;

    void removeSelectedNodesPopup();
    void removeSelectedNodes();

    GraphViewModel* m_viewModel{nullptr};
    GraphModel* m_model{nullptr};

    GraphTheme m_theme{};

    std::unordered_set<NodeIndex_t> m_selectedNodes{};

    NodeIndex_t m_hoveredNodeIndex{INVALID_NODE};
    NodeIndex_t m_draggingNodeIndex{INVALID_NODE};

    ImGuiMouseCursor m_currentMouseCursor : 4 {ImGuiMouseCursor_Arrow};

    bool m_drawGrid{true};
    bool m_drawQuadTree{false};
    bool m_drawMinMax{false};
    bool m_drawNodes{true};

    bool m_isDragSelecting{false};
    bool m_shouldOpenRemoveDialog{false};

    QuadTree m_quadTree;

    float m_minX{std::numeric_limits<float>::max()};
    float m_minY{std::numeric_limits<float>::max()};
    float m_maxX{-std::numeric_limits<float>::max()};
    float m_maxY{-std::numeric_limits<float>::max()};

    ImVec4 m_visibleRegionArea{};
    ImVec4 m_lastQueriedQuadTreeArea{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
    std::vector<NodeIndex_t> m_visibleNodesThisFrame{};

    ImVec2 m_selectStartPos{}, m_selectEndPos{};

    // OpenGL resources
    GLuint m_nodeTexture{};
    GLuint m_nodeOutlineTexture{};

    GLuint m_shaderProgram{};
    GLuint m_VAO{}, m_VBO{}, m_EBO{};
    GLuint m_quadVBO{}, m_instanceVBO{};

    struct NodeInstanceData {
        float m_x, m_y;
        ImU32 m_color;
    };

    std::vector<NodeInstanceData> m_nodeInstances{};
};

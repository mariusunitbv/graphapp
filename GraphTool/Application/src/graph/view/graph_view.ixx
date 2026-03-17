module;
#include <pch.h>

export module graph_view;

import graph_model;
import graph_view_model;

struct GraphTheme {
    ImU32 m_backgroundColor{IM_COL32(20, 20, 20, 255)};
    ImU32 m_gridColor{IM_COL32(35, 35, 35, 255)};
    ImU32 m_minMaxColor{IM_COL32(255, 0, 0, 255)};

    ImU32 m_nodeColor{IM_COL32(35, 35, 35, 255)};
    ImU32 m_nodeOutlineColor{IM_COL32(255, 255, 255, 255)};
    ImU32 m_selectedNodeOutlineColor{IM_COL32(89, 222, 18, 255)};
    ImU32 m_hoveredNodeOutlineColor{IM_COL32(18, 191, 222, 255)};
    ImU32 m_hoveredAndSelectedNodeOutlineColor{IM_COL32(18, 222, 130, 255)};
};

export class GraphView : public IGraphViewModelListener {
   public:
    ~GraphView();

    void initialize(const GraphModel* model, GraphViewModel* viewModel);
    void onSDLEvent(const SDL_Event& event);

    void renderUI();
    void renderScene();

    bool isFocusOnUI() const;

    void toggleFullScreen() { m_appFullScreen = !m_appFullScreen; }
    bool isFullScreen() const { return m_appFullScreen; }
    bool isFpsLimitEnabled() const { return m_isFpsLimitEnabled; }
    int getMaxFps() const { return m_maxFps; }
    int getVsyncMode() const;

   protected:
    void onFullDataUpdate() override;

    void onNodeSelected(NodeIndex_t nodeIndex) override;
    void onNodeDeselected(NodeIndex_t nodeIndex) override;

    void onNodeHover(NodeIndex_t nodeIndex) override;
    void onNodeUnhover(NodeIndex_t nodeIndex) override;

    void onNodeAdded(NodeIndex_t nodeIndex) override;
    void onNodeAddedToVisibleData(NodeIndex_t nodeIndex, uint32_t lookupIndex,
                                  VisibleData& visibleData) override;

   private:
    void initializeTextures();
    void initializeGL();
    void initializeNodesBuffersGL();
    void initializeEdgeGL();
    void initializeNodeGL();
    void initializeNodeFastGL();
    void initializeGridGL();

    GLuint compileShader(GLenum type, const char* source);

    void drawMenuBar();
    void drawStatusBar();
    void drawDeleteConfirmationDialog();
    void drawCenterOnNodeDialog();
    void drawFileView();
    void drawInspector();
    void drawSettings();

    void drawNodesIndexes(ImDrawList* drawList);
    void drawMinMax(ImDrawList* drawList);
    void drawSelectBox(ImDrawList* drawList);
    void drawMousePosition(ImDrawList* drawList);
    void drawVersion(ImDrawList* drawList);
    void drawWatermark(ImDrawList* drawList);

    void setupNodeBuffers();

    void drawBackground();
    void drawGrid();
    void drawEdges();
    void drawNodes();

    void colorNodes();
    void colorNode(NodeIndex_t nodeIndex);
    ImU32 getNodeColor(NodeIndex_t nodeIndex) const;
    ImU32 getOutlineColor(NodeIndex_t nodeIndex) const;

    bool shouldDrawNodes() const;

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};

    GraphTheme m_theme;

    bool m_drawGrid{true};
    bool m_drawMinMax{false};
    bool m_drawNodes{true};
    bool m_drawNodesOutline{true};
    bool m_drawEdges{true};

    bool m_isDeleteDialogOpen{false};
    bool m_isCenterOnNodeDialogOpen{false};
    bool m_isSettingsOpen{false};
    bool m_showDemoWindow{false};
    bool m_appFullScreen{false};

#ifndef __EMSCRIPTEN__
    bool m_isFpsLimitEnabled{true};
    int m_maxFps{120};
#else
    static constexpr int m_maxFps{0};
    static constexpr bool m_isFpsLimitEnabled{false};
#endif

    int m_vsyncMode{0};
    int m_outlineThickness{2};
    float m_gridCellSize{100.f};
    int m_nodeCutoffZoom{55};
    int m_graphTextFontIndex{1};

    GLfloat m_pointSizeRange[2]{};
    GLuint m_unitbvLogoTexture{};
    GLint m_maxTextureSize{};

    GLuint m_nodePositionTBO{};
    GLuint m_nodeColorTBO{};

    GLuint m_nodePositionTex{};
    GLuint m_nodeColorTex{};

    struct GLObject {
        ~GLObject() {
            if (m_VAO) {
                glDeleteVertexArrays(1, &m_VAO);
            }

            if (m_VBO) {
                glDeleteBuffers(1, &m_VBO);
            }

            if (m_shaderProgram) {
                glDeleteProgram(m_shaderProgram);
            }

            if (m_quadVBO) {
                glDeleteBuffers(1, &m_quadVBO);
            }

            if (m_EBO) {
                glDeleteBuffers(1, &m_EBO);
            }
        }

        GLuint m_VAO{};
        GLuint m_VBO{};
        GLuint m_shaderProgram{};

        GLuint m_quadVBO{};
        GLuint m_EBO{};
    };

    GLObject m_nodeGLObject;
    GLObject m_nodeFastGLObject;
    GLObject m_edgeGLObject;

    struct NodeUniforms {
        GLint m_nodePosition{-1};
        GLint m_nodeColor{-1};
        GLint m_nodeRadius{-1};
        GLint m_screenSize{-1};
        GLint m_cameraPos{-1};
        GLint m_cameraZoom{-1};
        GLint m_nodeThickness{-1};

#ifdef __EMSCRIPTEN__
        GLint m_textureWidth{-1};
#endif
    };

    NodeUniforms m_nodeUniforms;
    NodeUniforms m_fastNodeUniforms;

    struct EdgeUniforms {
        GLint m_nodePosition{-1};
        GLint m_nodeColor{-1};
        GLint m_screenSize{-1};
        GLint m_cameraPos{-1};
        GLint m_cameraZoom{-1};

#ifdef __EMSCRIPTEN__
        GLint m_textureWidth{-1};
#endif
    };

    EdgeUniforms m_edgeUniforms;

    struct GridLineInstanceData {
        Vector2D m_worldStart{};
        Vector2D m_worldEnd{};
        float m_thickness{};
    };

    GLObject m_gridGLObject;
    GLint m_gridUniformScreenSize{-1};
    GLint m_gridUniformColor{-1};

    int m_lastVisibleEdgesCount{0};

    bool m_edgesBufferDirty{false};
    bool m_nodesColorDirty{false};
    bool m_nodesPositionDirty{false};
};

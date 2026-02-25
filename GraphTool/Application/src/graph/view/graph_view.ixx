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

export class GraphView {
   public:
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

    void setSmallNodeFont(ImFont* font) { m_smallNodeFont = font; }
    void setMediumNodeFont(ImFont* font) { m_mediumNodeFont = font; }
    void setLargeNodeFont(ImFont* font) { m_largeNodeFont = font; }

   private:
    void initializeGL();
    void initializeNodeGL();
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

    void drawBackground();
    void drawGrid();
    void drawNodes();

    void colorVisibleNodes(std::vector<VisibleNode>& visibleNodes);
    ImU32 getNodeColor(const Node* node) const;
    ImU32 getOutlineColor(const Node* node) const;

    bool shouldDrawNodes() const;

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};

    GraphTheme m_theme;

    bool m_drawGrid{true};
    bool m_drawMinMax{false};
    bool m_drawNodes{true};
    bool m_drawNodesOutline{true};

    bool m_isFpsLimitEnabled{true};
    bool m_isDeleteDialogOpen{false};
    bool m_isCenterOnNodeDialogOpen{false};
    bool m_isSettingsOpen{false};
    bool m_showDemoWindow{false};
    bool m_appFullScreen{false};

    int m_maxFps{360};
    int m_vsyncMode{0};
    int m_outlineThickness{1};
    float m_gridCellSize{100.f};
    int m_nodeCutoffZoom{10};

    ImFont* m_smallNodeFont{nullptr};
    ImFont* m_mediumNodeFont{nullptr};
    ImFont* m_largeNodeFont{nullptr};

    struct GLObject {
        ~GLObject() {
            if (m_VAO) {
                glDeleteVertexArrays(1, &m_VAO);
            }

            if (m_VBO) {
                glDeleteBuffers(1, &m_VBO);
            }

            if (m_instanceVBO) {
                glDeleteBuffers(1, &m_instanceVBO);
            }

            if (m_shaderProgram) {
                glDeleteProgram(m_shaderProgram);
            }
        }

        GLuint m_VAO{};
        GLuint m_VBO{};
        GLuint m_instanceVBO{};
        GLuint m_shaderProgram{};
    };

    GLObject m_nodeGLObject;

    struct GridLineInstanceData {
        Vector2D m_worldStart{};
        Vector2D m_worldEnd{};
        ImU32 m_color{};
        float m_thickness{};
    };

    GLObject m_gridGLObject;
};

module;
#include <pch.h>

export module graph_renderer;

export import graph_model;
export import graph_view_model;
export import graph_view_settings;

import node_states;
import algorithm;

export class GraphRenderer : public IGraphViewModelListener {
   public:
    ~GraphRenderer();

    void initialize(const GraphViewSettings* viewSettings);
    void preRenderUpdate(const GraphModel* model, GraphViewModel* viewModel);

    // Render using ImGui draw lists.
    void render();

    // Render using native GL calls.
    void renderNative();

   protected:
    void onFullDataUpdate() override;

    void onNodeSelected(NodeIndex_t nodeIndex) override;
    void onNodeDeselected(NodeIndex_t nodeIndex) override;

    void onNodeHover(NodeIndex_t nodeIndex) override;
    void onNodeUnhover(NodeIndex_t nodeIndex) override;

    void onNodeAdded(NodeIndex_t nodeIndex) override;
    void onNodeAddedToVisibleData(NodeIndex_t nodeIndex, uint32_t lookupIndex,
                                  VisibleData& visibleData) override;

    void onNodeStateChange(NodeIndex_t nodeIndex, NodeState newState,
                           AlgorithmType algorithmType) override;

    void onAlgorithmStarted() override;
    void onAlgorithmAborted() override;

    void onAlgorithmPseudocodeEvent(const std::string_view event) override {}

   private:
    void initializeNodesBuffersGL();
    void initializeEdgeGL();
    void initializeNodeGL();
    void initializeNodeFastGL();
    void initializeGridGL();

    GLuint compileShader(GLenum type, const char* source);

    void setupNodeBuffers();
    void drawBackground();
    void drawGrid();
    void drawEdges();
    void drawNodes();

    void drawHighlightedEdges(ImDrawList* drawList);
    void drawCosts(ImDrawList* drawList);
    void drawNodesIndexes(ImDrawList* drawList);
    void drawMinMax(ImDrawList* drawList);
    void drawSelectBox(ImDrawList* drawList);
    void drawMousePosition(ImDrawList* drawList);

    bool shouldDrawNodes() const;
    bool isFocusOnUI() const;

    void colorNodes();
    void colorNode(NodeIndex_t nodeIndex);
    ImU32 getNodeColor(NodeIndex_t nodeIndex) const;
    ImU32 getNodeColorAlgorithm(NodeIndex_t nodeIndex, AlgorithmType algorithmType,
                                int nodeAlpha) const;
    ImU32 getOutlineColor(NodeIndex_t nodeIndex) const;

    const GraphModel* m_model{nullptr};
    GraphViewModel* m_viewModel{nullptr};
    const GraphViewSettings* m_viewSettings{nullptr};

    GLfloat m_pointSizeRange[2]{};
    GLint m_maxTextureSize{};

    GLuint m_nodePositionTBO{};
    GLuint m_nodeColorTBO{};
    GLuint m_selfLoopsTBO{};

    GLuint m_nodePositionTex{};
    GLuint m_nodeColorTex{};
    GLuint m_selfLoopsTex{};

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
        GLint m_selfLoops{-1};
        GLint m_selfLoopsSize{-1};

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
        GLint m_nodeRadius{-1};
        GLint m_algorithmCreated{-1};

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
    bool m_selfLoopsDirty{false};
};

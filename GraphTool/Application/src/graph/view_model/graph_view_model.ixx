module;
#include <pch.h>

export module graph_view_model;
export import graph_view_model_defines;
export import graph_view_model_listener;

import graph_model;

export class GraphViewModel {
   public:
    GraphViewModel() = default;

    GraphViewModel(const GraphViewModel&) = delete;
    GraphViewModel& operator=(const GraphViewModel&) = delete;

    GraphViewModel(GraphViewModel&& rhs) noexcept;
    GraphViewModel& operator=(GraphViewModel&& rhs) noexcept;

    void onSDLEvent(const SDL_Event& event, bool focusOnUI);
    void preRenderUpdate();

    void setModel(GraphModel* model);
    GraphModel* getModel() const;

    void addListener(IGraphViewModelListener* listener);

    Vector2D getSceneSize() const;
    void updateSceneSize(float displayWidth, float displayHeight);

    const std::vector<Vector2D>& getVisibleNodesPositions() const;
    std::vector<NodeColorInfo>& getVisibleNodesColors();

    const std::vector<NodeIndex_t>& getVisibleNodes() const;
    const std::vector<VisibleEdge>& getVisibleEdges() const;
    const std::vector<uint8_t>& getVisibleLoops() const;

    void refreshVisibleData();

    NodeIndex_t getHoveredNodeIndex() const;
    size_t getSelectedNodesCount() const;
    bool isNodeSelected(NodeIndex_t nodeIndex) const;

    std::optional<uint32_t> getLookupIndex(NodeIndex_t nodeIndex) const;

    float getZoomFactor() const;
    void setZoomFactor(float zoom);

    int getMaxNodesPerCellBase() const;
    void setMaxNodesPerCellBase(int maxNodes);

    float getNodeCondensationPercentage() const;
    void setNodeCondensationPercentage(float percentage);

    int getEdgeDrawPercentage() const;
    void setEdgeDrawPercentage(int percentage);

    int getMaxVisibleNodes() const;
    void setMaxVisibleNodes(int maxNodes);

    float getNodesRadius() const;
    void setNodesRadius(float radius);

    bool shouldCondensateNodesLowZoom() const;
    void setShouldCondensateNodesLowZoom(bool shouldCondensate);

    float getOverscanFactor() const;
    void setOverscanFactor(float factor);

    Vector2D getCameraPosition() const;
    BoundingBox2D getVisibleRegionWorld(Vector2D additionalPadding = {}) const;

    Vector2D worldToScreen(Vector2D worldPos) const;
    Vector2D screenToWorld(Vector2D screenPos) const;

    void removeSelectedNodes();

    bool isSelectingUsingBox() const;
    const BoundingBox2D& getSelectBoxBounds() const;

    void centerOnNode(NodeIndex_t nodeIndex);

    bool isRunningUpdate() const;
    void cancelRunningUpdate();

    void addEdge(NodeIndex_t from, NodeIndex_t to, int weight);
    void removeEdge(NodeIndex_t from, NodeIndex_t to);

   private:
    void onSceneResize(float displayWidth, float displayHeight);
    void onCameraPan(float deltaX, float deltaY);
    void onCameraZoom(float deltaZoom, float cursorX, float cursorY);

    void onMouseClick(float cursorX, float cursorY, bool ctrlPressed);
    void onMouseMove(float cursorX, float cursorY);

    void setSelectBoxStart(float cursorX, float cursorY);
    void setSelectBoxEnd(float cursorX, float cursorY);
    void selectNodesInBox();
    void selectNode(NodeIndex_t nodeIndex);
    void deselectNode(NodeIndex_t nodeIndex);
    void deselectAllNodes();

    void updateVisibleNodes(VisibleData& visibleData);
    void updateVisibleEdges(VisibleData& visibleData);
    void onVisibleNode(NodeIndex_t nodeIndex, VisibleData& visibleData);
    void setupVisibleNodes(VisibleData& visibleData);

    void clampCameraPositionInBounds();
    void updateVisibleRegion();
    void invalidateVisibleData();

    GraphModel* m_model{nullptr};

    GraphCamera m_camera{};
    Vector2D m_displaySize{};

    BoundingBox2D m_visibleRegionArea{};
    BoundingBox2D m_lastQueryRegionArea{};

    VisibleData m_visibleData{};
    VisibleData m_cachedVisibleData{};

    std::unordered_set<NodeIndex_t> m_selectedNodes{};

    int m_maxNodesPerCellPercentage{85};
    float m_nodeCondensationFactor{0.35f};
    int m_edgeDrawPercentage{100};
    int m_maxVisibleNodes{7'500'000};
    float m_nodesRadius{28.f};
    float m_overscanFactor{0.75f};

    NodeIndex_t m_hoveredNodeIndex{INVALID_NODE};
    bool m_isSelectingUsingBox{false};
    bool m_shouldBlockMouseLeftClick{false};
    bool m_shouldCondensateNodesLowZoom{true};
    bool m_shouldUseCachedVisibleNodes{false};

#ifdef __EMSCRIPTEN__
    static constexpr bool m_isUpdateFutureRunning{false};
#else
    bool m_isUpdateFutureRunning{false};
    std::future<VisibleData> m_updateFuture{};
#endif

    std::chrono::steady_clock::time_point m_lastSelectBoxQueryTime{};
    Vector2D m_selectBoxStartWorldPos{};
    BoundingBox2D m_selectBoxBounds{};

    std::unordered_map<SDL_FingerID, SDL_TouchFingerEvent> m_activeFingers;
    float m_lastZoomDelta{};

    std::vector<IGraphViewModelListener*> m_listeners{};
};

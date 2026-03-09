module;
#include <pch.h>

export module graph_view_model;
export import graph_view_model_defines;
export import graph_view_model_listener;

import graph_model;

export class GraphViewModel {
   public:
    void initialize(GraphModel* model, float displayWidth, float displayHeight);
    void onSDLEvent(const SDL_Event& event, bool focusOnUI);
    void preRenderUpdate();

    void addListener(IGraphViewModelListener* listener);

    const std::vector<Vector2D>& getVisibleNodesPositions() const;
    std::vector<NodeColorInfo>& getVisibleNodesColors();
    const std::vector<NodeIndex_t>& getVisibleNodesIndexes() const;

    const std::vector<VisibleNode>& getVisibleNodes() const;
    const std::vector<VisibleEdge>& getVisibleEdges() const;

    NodeIndex_t getHoveredNodeIndex() const;
    size_t getSelectedNodesCount() const;
    bool isNodeSelected(NodeIndex_t nodeIndex) const;

    bool isValidLookupIndex(NodeIndex_t nodeIndex, uint32_t lookupIndex) const;

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

    Vector2D getCameraPosition() const;
    BoundingBox2D getVisibleRegionWorld(Vector2D additionalPadding = {}) const;

    Vector2D worldToScreen(Vector2D worldPos) const;
    Vector2D screenToWorld(Vector2D screenPos) const;

    void removeSelectedNodes();

    bool isSelectingUsingBox() const;
    const BoundingBox2D& getSelectBoxBounds() const;

    void centerOnNode(NodeIndex_t nodeIndex);

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

    void clampCameraPositionInBounds();
    void updateVisibleRegion();
    void invalidateVisibleData();

    void addSampleNodes();

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

    NodeIndex_t m_hoveredNodeIndex{INVALID_NODE};
    bool m_isSelectingUsingBox{false};
    bool m_shouldBlockMouseLeftClick{false};
    bool m_shouldCondensateNodesLowZoom{true};
    bool m_shouldUseCachedVisibleNodes{false};

    std::chrono::steady_clock::time_point m_lastSelectBoxQueryTime{};
    Vector2D m_selectBoxStartWorldPos{};
    BoundingBox2D m_selectBoxBounds{};

    std::unordered_map<SDL_FingerID, SDL_TouchFingerEvent> m_activeFingers;
    float m_lastZoomDelta{};

    std::vector<IGraphViewModelListener*> m_listeners{};
};

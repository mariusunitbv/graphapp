module;
#include <pch.h>

export module graph_view_model;

import graph_model;

FORWARD_DECLARE_CLASS(GraphView);

export struct GraphCamera {
    Vector2D m_position{};
    float m_zoom{1.0f};
};

export class GraphViewModel {
   public:
    void initialize(GraphModel* model, GraphView* view, float displayWidth, float displayHeight);
    void onSDLEvent(const SDL_Event& event);
    void preRenderUpdate();

    std::vector<VisibleNode>& getVisibleNodes();
    NodeIndex_t getHoveredNodeIndex() const;
    size_t getSelectedNodesCount() const;
    bool isNodeSelected(NodeIndex_t nodeIndex) const;

    float getZoomFactor() const;
    Vector2D getCameraPosition() const;
    const BoundingBox2D& getVisibleRegion() const;
    BoundingBox2D getVisibleRegionWorldCoordonates(Vector2D additionalPadding = {}) const;

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

    void updateVisibleRegion();
    void invalidateVisibleNodesCache();

    GraphModel* m_model{nullptr};
    GraphView* m_view{nullptr};

    GraphCamera m_camera{};
    Vector2D m_sceneSize{};

    BoundingBox2D m_visibleRegionArea{};

    BoundingBox2D m_lastQueryRegionArea{};
    std::vector<VisibleNode> m_visibleNodes{};
    std::unordered_set<NodeIndex_t> m_selectedNodes;

    NodeIndex_t m_hoveredNodeIndex{INVALID_NODE};
    bool m_isSelectingUsingBox{false};

    std::chrono::steady_clock::time_point m_lastSelectBoxQueryTime{};
    Vector2D m_selectBoxStartWorldPos{};
    BoundingBox2D m_selectBoxBounds{};
};

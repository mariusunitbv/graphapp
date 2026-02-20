module;
#include <pch.h>

export module graph_view_model;

import graph_model;
import math;

export struct GraphCamera {
    Vector2D m_position{};
    float m_zoom{1.0f};
};

export class GraphViewModel {
   public:
    void initialize(GraphModel* model, float displayWidth, float displayHeight);
    void onSDLEvent(const SDL_Event& event);

    const std::vector<VisibleNode>& getVisibleNodes();

    float getZoomFactor() const;
    Vector2D getCameraPosition() const;
    const BoundingBox2D& getVisibleRegion() const;

    Vector2D worldToScreen(Vector2D worldPos) const;
    Vector2D screenToWorld(Vector2D screenPos) const;

   private:
    void onSceneResize(float displayWidth, float displayHeight);
    void onCameraPan(float deltaX, float deltaY);
    void onCameraZoom(float deltaZoom, float cursorX, float cursorY);

    void updateVisibleRegion();
    void invalidateVisibleNodesCache();

    GraphModel* m_model{nullptr};

    GraphCamera m_camera{};
    Vector2D m_sceneSize{};

    BoundingBox2D m_visibleRegionArea{};

    BoundingBox2D m_lastQueryRegionArea{};
    std::vector<VisibleNode> m_visibleNodes{};
};

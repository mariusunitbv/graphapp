module;
#include <pch.h>

export module graph_view_model;

import graph_model;

export struct GraphCamera {
    float m_x{0.0f}, m_y{0.0f};
    float m_zoom{1.0f};
};

export class GraphViewModel {
   public:
    void setModel(GraphModel* model);

    float getZoomFactor() const;
    std::pair<float, float> getCameraPosition() const;

    void onCameraPan(float deltaX, float deltaY);
    void onCameraZoom(float deltaZoom, float cursorX, float cursorY, float displayWidth,
                      float displayHeight);

    void onNodeMove(NodeIndex_t nodeIndex, float deltaX, float deltaY);

    std::pair<float, float> worldToScreen(float worldX, float worldY, float displayWidth,
                                          float displayHeight) const;
    std::pair<float, float> screenToWorld(float screenX, float screenY, float displayWidth,
                                          float displayHeight) const;

   private:
    GraphModel* m_model{nullptr};
    GraphCamera m_camera;
};

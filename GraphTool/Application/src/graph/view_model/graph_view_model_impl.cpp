module;
#include <pch.h>

module graph_view_model;

float GraphViewModel::getZoomFactor() const { return m_camera.m_zoom; }

std::pair<float, float> GraphViewModel::getCameraPosition() const {
    return {m_camera.m_x, m_camera.m_y};
}

void GraphViewModel::onCameraPan(float deltaX, float deltaY) {
    m_camera.m_x += deltaX / m_camera.m_zoom;
    m_camera.m_y += deltaY / m_camera.m_zoom;
}

void GraphViewModel::onCameraZoom(float deltaZoom, float cursorX, float cursorY, float displayWidth,
                                  float displayHeight) {
    const auto world = screenToWorld(cursorX, cursorY, displayWidth, displayHeight);

    m_camera.m_zoom += (deltaZoom > 0) ? 0.1f : -0.1f;
    m_camera.m_zoom = std::clamp(m_camera.m_zoom, 0.1f, 5.f);

    m_camera.m_x = world.first - (cursorX - displayWidth * 0.5f) / m_camera.m_zoom;
    m_camera.m_y = world.second - (cursorY - displayHeight * 0.5f) / m_camera.m_zoom;
}

void GraphViewModel::onNodeMove(NodeIndex_t nodeIndex, float deltaX, float deltaY) {}

std::pair<float, float> GraphViewModel::worldToScreen(float worldX, float worldY,
                                                      float displayWidth,
                                                      float displayHeight) const {
    return {(worldX - m_camera.m_x) * m_camera.m_zoom + displayWidth * 0.5f,
            (worldY - m_camera.m_y) * m_camera.m_zoom + displayHeight * 0.5f};
}

std::pair<float, float> GraphViewModel::screenToWorld(float screenX, float screenY,
                                                      float displayWidth,
                                                      float displayHeight) const {
    return {(screenX - displayWidth * 0.5f) / m_camera.m_zoom + m_camera.m_x,
            (screenY - displayHeight * 0.5f) / m_camera.m_zoom + m_camera.m_y};
}

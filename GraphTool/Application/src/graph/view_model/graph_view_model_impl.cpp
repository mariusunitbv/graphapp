module;
#include <pch.h>

module graph_view_model;

void GraphViewModel::initialize(GraphModel* model, float displayWidth, float displayHeight) {
    m_model = model;

    onSceneResize(displayWidth, displayHeight);
}

void GraphViewModel::onSDLEvent(const SDL_Event& event) {
    const auto lShiftPressed = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    const auto lCtrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    const auto lAltPressed = (SDL_GetModState() & SDL_KMOD_ALT) != 0;

    switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            onSceneResize((float)event.window.data1, (float)event.window.data2);
            break;
        case SDL_EVENT_MOUSE_MOTION: {
            const auto leftMouseButtonDown = event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
            const auto rightMouseButtonDown =
                event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);

            if (rightMouseButtonDown || (leftMouseButtonDown && lAltPressed)) {
                onCameraPan(-event.motion.xrel, -event.motion.yrel);
            }

        } break;
        case SDL_EVENT_MOUSE_WHEEL:
            onCameraZoom(event.wheel.y, event.wheel.mouse_x, event.wheel.mouse_y);
            break;
    }
}

const std::vector<VisibleNode>& GraphViewModel::getVisibleNodes() {
    if (m_lastQueryRegionArea.contains(m_visibleRegionArea)) {
        return m_visibleNodes;
    }

    const auto extraMargin = m_sceneSize * 0.3f;
    m_lastQueryRegionArea = {
        screenToWorld(-extraMargin),
        screenToWorld(m_sceneSize + extraMargin),
    };

    const auto& nodes = m_model->getNodes();
    m_visibleNodes = m_model->getQuadTree()->query(nodes, m_lastQueryRegionArea, nodes.size());

    return m_visibleNodes;
}

float GraphViewModel::getZoomFactor() const { return m_camera.m_zoom; }

Vector2D GraphViewModel::getCameraPosition() const { return m_camera.m_position; }

const BoundingBox2D& GraphViewModel::getVisibleRegion() const { return m_visibleRegionArea; }

Vector2D GraphViewModel::worldToScreen(Vector2D worldPos) const {
    return (worldPos - m_camera.m_position) * m_camera.m_zoom + m_sceneSize * 0.5f;
}

Vector2D GraphViewModel::screenToWorld(Vector2D screenPos) const {
    return (screenPos - m_sceneSize * 0.5f) / m_camera.m_zoom + m_camera.m_position;
}

void GraphViewModel::onSceneResize(float displayWidth, float displayHeight) {
    m_sceneSize = {displayWidth, displayHeight};

    invalidateVisibleNodesCache();
    updateVisibleRegion();
}

void GraphViewModel::onCameraPan(float deltaX, float deltaY) {
    m_camera.m_position.m_x += deltaX / m_camera.m_zoom;
    m_camera.m_position.m_y += deltaY / m_camera.m_zoom;

    updateVisibleRegion();
}

void GraphViewModel::onCameraZoom(float deltaZoom, float cursorX, float cursorY) {
    const auto world = screenToWorld({cursorX, cursorY});

    m_camera.m_zoom += (deltaZoom > 0) ? 0.1f : -0.1f;
    m_camera.m_zoom = std::clamp(m_camera.m_zoom, 0.1f, 5.f);

    m_camera.m_position.m_x = world.m_x - (cursorX - m_sceneSize.m_x * 0.5f) / m_camera.m_zoom;
    m_camera.m_position.m_y = world.m_y - (cursorY - m_sceneSize.m_y * 0.5f) / m_camera.m_zoom;

    invalidateVisibleNodesCache();
    updateVisibleRegion();
}

void GraphViewModel::updateVisibleRegion() {
    m_visibleRegionArea = {screenToWorld({0.f, 0.f}), screenToWorld(m_sceneSize)};
}

void GraphViewModel::invalidateVisibleNodesCache() {
    m_visibleNodes.clear();
    m_lastQueryRegionArea = {};
}

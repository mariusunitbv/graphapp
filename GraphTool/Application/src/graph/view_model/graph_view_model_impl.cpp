module;
#include <pch.h>

module graph_view_model;

import graph_view;

void GraphViewModel::initialize(GraphModel* model, GraphView* view, float displayWidth,
                                float displayHeight) {
    m_model = model;
    m_view = view;

    onSceneResize(displayWidth, displayHeight);
}

void GraphViewModel::onSDLEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        onSceneResize((float)event.window.data1, (float)event.window.data2);
        return;
    }

    if (m_view->isFocusOnUI()) {
        return;
    }

    const auto lShiftPressed = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    const auto lCtrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    const auto lAltPressed = (SDL_GetModState() & SDL_KMOD_ALT) != 0;

    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT && !lAltPressed) {
                if (lShiftPressed) {
                    m_isSelectingUsingBox = true;
                    setSelectBoxStart(event.button.x, event.button.y);
                } else {
                    onMouseClick(event.button.x, event.button.y, lCtrlPressed);
                }
            }

            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_isSelectingUsingBox = false;
            }

            break;
        case SDL_EVENT_MOUSE_MOTION: {
            const auto leftMouseButtonDown = event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
            const auto rightMouseButtonDown =
                event.motion.state & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);

            if (rightMouseButtonDown || (leftMouseButtonDown && lAltPressed)) {
                onCameraPan(-event.motion.xrel, -event.motion.yrel);
            } else if (leftMouseButtonDown && m_isSelectingUsingBox) {
                setSelectBoxEnd(event.motion.x, event.motion.y);
            }

            onMouseMove(event.motion.x, event.motion.y);

        } break;
        case SDL_EVENT_MOUSE_WHEEL:
            onCameraZoom(event.wheel.y, event.wheel.mouse_x, event.wheel.mouse_y);
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
                case SDLK_LEFT:
                    onCameraPan(-10.f, 0.f);
                    break;
                case SDLK_RIGHT:
                    onCameraPan(10.f, 0.f);
                    break;
                case SDLK_UP:
                    if (lCtrlPressed) {
                        onCameraZoom(1.f, m_sceneSize.m_x * 0.5f, m_sceneSize.m_y * 0.5f);
                    } else {
                        onCameraPan(0.f, -10.f);
                    }

                    break;
                case SDLK_DOWN:
                    if (lCtrlPressed) {
                        onCameraZoom(-1.f, m_sceneSize.m_x * 0.5f, m_sceneSize.m_y * 0.5f);
                    } else {
                        onCameraPan(0.f, 10.f);
                    }

                    break;
                case SDLK_DELETE:
                    if (!m_selectedNodes.empty()) {
                        m_view->openDeleteConfirmationDialog();
                    }

                    break;
                case SDLK_C:
                    m_view->openCenterOnNodeDialog();
                    break;
                case SDLK_G:
                    m_view->toggleGrid();
                    break;
                case SDLK_N:
                    m_view->toggleDrawNodes();
                    break;
            }

            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_LSHIFT) {
                m_isSelectingUsingBox = false;
            }

            break;
    }
}

void GraphViewModel::preRenderUpdate() { selectNodesInBox(); }

std::vector<VisibleNode>& GraphViewModel::getVisibleNodes() {
    if (m_lastQueryRegionArea.contains(m_visibleRegionArea)) {
        return m_visibleNodes;
    }

    const auto extraMargin = m_sceneSize * 0.3f;
    m_lastQueryRegionArea = {
        screenToWorld(-extraMargin),
        screenToWorld(m_sceneSize + extraMargin),
    };

    m_visibleNodes = m_model->queryNodes(m_lastQueryRegionArea);
    return m_visibleNodes;
}

NodeIndex_t GraphViewModel::getHoveredNodeIndex() const { return m_hoveredNodeIndex; }

size_t GraphViewModel::getSelectedNodesCount() const { return m_selectedNodes.size(); }

bool GraphViewModel::isNodeSelected(NodeIndex_t nodeIndex) const {
    return m_selectedNodes.contains(nodeIndex);
}

float GraphViewModel::getZoomFactor() const { return m_camera.m_zoom; }

Vector2D GraphViewModel::getCameraPosition() const { return m_camera.m_position; }

const BoundingBox2D& GraphViewModel::getVisibleRegion() const { return m_visibleRegionArea; }

BoundingBox2D GraphViewModel::getVisibleRegionWorldCoordonates(Vector2D additionalPadding) const {
    const auto topLeft = screenToWorld(-additionalPadding);
    const auto bottomRight = screenToWorld(m_sceneSize + additionalPadding);

    const auto& graphBounds = m_model->getQuadTree()->getBounds();

    return {max(topLeft, graphBounds.m_min), min(bottomRight, graphBounds.m_max)};
}

Vector2D GraphViewModel::worldToScreen(Vector2D worldPos) const {
    return (worldPos - m_camera.m_position) * m_camera.m_zoom + m_sceneSize * 0.5f;
}

Vector2D GraphViewModel::screenToWorld(Vector2D screenPos) const {
    return (screenPos - m_sceneSize * 0.5f) / m_camera.m_zoom + m_camera.m_position;
}

void GraphViewModel::removeSelectedNodes() {
    m_model->removeNodes(m_selectedNodes);

    invalidateVisibleNodesCache();

    m_hoveredNodeIndex = INVALID_NODE;
    m_selectedNodes.clear();
}

bool GraphViewModel::isSelectingUsingBox() const { return m_isSelectingUsingBox; }

const BoundingBox2D& GraphViewModel::getSelectBoxBounds() const { return m_selectBoxBounds; }

void GraphViewModel::centerOnNode(NodeIndex_t nodeIndex) {
    const auto node = m_model->getNode(nodeIndex);
    if (!node) {
        return;
    }

    const auto nodeScreenPos = worldToScreen(node->m_worldPos);

    m_camera.m_position = node->m_worldPos;
    m_hoveredNodeIndex = nodeIndex;

    updateVisibleRegion();
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

void GraphViewModel::onMouseClick(float cursorX, float cursorY, bool ctrlPressed) {
    if (m_hoveredNodeIndex != INVALID_NODE) {
        if (ctrlPressed) {
            if (m_selectedNodes.contains(m_hoveredNodeIndex)) {
                m_selectedNodes.erase(m_hoveredNodeIndex);
            } else {
                m_selectedNodes.insert(m_hoveredNodeIndex);
            }
        } else {
            m_selectedNodes.clear();
            m_selectedNodes.insert(m_hoveredNodeIndex);
        }

        return;
    }

    if (!m_selectedNodes.empty()) {
        m_selectedNodes.clear();
        return;
    }

    if (m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}), true, 2 * NODE_RADIUS)) {
        return;
    }

    const auto worldPos = screenToWorld({cursorX, cursorY});
    m_model->addNode(worldPos);

    const auto lastNodeIndex = m_model->getLastNodeIndex();
    const auto lastNode = m_model->getNode(lastNodeIndex);

    m_visibleNodes.emplace_back(lastNode->m_worldPos, lastNode->getABGR(), lastNodeIndex);
    m_hoveredNodeIndex = lastNodeIndex;
}

void GraphViewModel::onMouseMove(float cursorX, float cursorY) {
    const auto hoveredNode = m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}));
    if (!hoveredNode) {
        m_hoveredNodeIndex = INVALID_NODE;
        return;
    }

    m_hoveredNodeIndex = hoveredNode->m_index;
}

void GraphViewModel::setSelectBoxStart(float cursorX, float cursorY) {
    m_selectBoxStartWorldPos = screenToWorld({cursorX, cursorY});
    m_selectBoxBounds = {m_selectBoxStartWorldPos, m_selectBoxStartWorldPos};
}

void GraphViewModel::setSelectBoxEnd(float cursorX, float cursorY) {
    const auto worldPos = screenToWorld({cursorX, cursorY});
    m_selectBoxBounds = BoundingBox2D{min(m_selectBoxStartWorldPos, worldPos),
                                      max(m_selectBoxStartWorldPos, worldPos)};
}

void GraphViewModel::selectNodesInBox() {
    if (!m_isSelectingUsingBox) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto timeSinceLastQuery =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastSelectBoxQueryTime);
    if (timeSinceLastQuery < std::chrono::milliseconds(50)) {
        return;
    }

    const auto ctrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    if (!ctrlPressed) {
        m_selectedNodes.clear();
    }

    const auto queryResult = m_model->queryNodes(m_selectBoxBounds);
    const auto indices =
        queryResult | std::views::transform([](const VisibleNode& vn) { return vn.m_index; });

    m_selectedNodes.reserve(m_selectedNodes.size() + queryResult.size());
    m_selectedNodes.insert(indices.begin(), indices.end());

    m_lastSelectBoxQueryTime = now;
}

void GraphViewModel::updateVisibleRegion() {
    m_visibleRegionArea = {screenToWorld({0.f, 0.f}), screenToWorld(m_sceneSize)};
}

void GraphViewModel::invalidateVisibleNodesCache() {
    m_visibleNodes.clear();
    m_lastQueryRegionArea = {};
}

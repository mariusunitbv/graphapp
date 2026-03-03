module;
#include <pch.h>

module graph_view_model;

void GraphViewModel::initialize(GraphModel* model, float displayWidth, float displayHeight) {
    m_model = model;

    onSceneResize(displayWidth, displayHeight);
    addSampleNodes();
}

void GraphViewModel::onSDLEvent(const SDL_Event& event, bool focusOnUI) {
    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        onSceneResize((float)event.window.data1, (float)event.window.data2);
        return;
    }

    if (focusOnUI) {
        m_activeFingers.clear();
        return;
    }

    const auto lShiftPressed = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    const auto lCtrlPressed = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    const auto lAltPressed = (SDL_GetModState() & SDL_KMOD_ALT) != 0;

    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT && !lAltPressed && lShiftPressed) {
                if (!lCtrlPressed) {
                    deselectAllNodes();
                }

                m_isSelectingUsingBox = true;
                setSelectBoxStart(event.button.x, event.button.y);
            }

            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.which == SDL_PEN_MOUSEID) {
                break;
            }

            if (event.button.button == SDL_BUTTON_LEFT) {
                m_isSelectingUsingBox = false;

                if (m_shouldBlockMouseLeftClick) {
                    m_shouldBlockMouseLeftClick = false;
                } else {
                    if (!lShiftPressed && !lAltPressed && m_activeFingers.size() < 2) {
                        onMouseClick(event.button.x, event.button.y, lCtrlPressed);
                    }
                }
            }

            break;
        case SDL_EVENT_MOUSE_MOTION: {
            if (event.motion.which == SDL_PEN_MOUSEID) {
                break;
            }

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
                        onCameraZoom(1.f, m_displaySize.m_x * 0.5f, m_displaySize.m_y * 0.5f);
                    } else {
                        onCameraPan(0.f, -10.f);
                    }

                    break;
                case SDLK_DOWN:
                    if (lCtrlPressed) {
                        onCameraZoom(-1.f, m_displaySize.m_x * 0.5f, m_displaySize.m_y * 0.5f);
                    } else {
                        onCameraPan(0.f, 10.f);
                    }

                    break;
            }

            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_LSHIFT && m_isSelectingUsingBox) {
                m_isSelectingUsingBox = false;
                m_shouldBlockMouseLeftClick = true;
            }

            break;
        case SDL_EVENT_FINGER_DOWN:
            m_activeFingers[event.tfinger.fingerID] = event.tfinger;
            break;
        case SDL_EVENT_FINGER_UP:
            m_activeFingers.erase(event.tfinger.fingerID);
            break;
        case SDL_EVENT_FINGER_MOTION:
            const auto& finger = event.tfinger;
            m_activeFingers[finger.fingerID] = finger;

            {
                constexpr auto MOVEMENT_THRESHOLD_PX = 3.f;

                const auto dx = finger.dx * m_displaySize.m_x;
                const auto dy = finger.dy * m_displaySize.m_y;

                if (dx * dx + dy * dy > MOVEMENT_THRESHOLD_PX * MOVEMENT_THRESHOLD_PX) {
                    m_shouldBlockMouseLeftClick = true;
                }
            }

            if (m_activeFingers.size() == 1) {
                const auto dx = finger.dx * m_displaySize.m_x;
                const auto dy = finger.dy * m_displaySize.m_y;
                onCameraPan(-dx, -dy);
            } else if (m_activeFingers.size() == 2) {
                constexpr float ZOOM_THRESHOLD = 0.01f;

                auto it = m_activeFingers.begin();
                const auto& f1 = it->second;
                ++it;
                const auto& f2 = it->second;

                const auto dx = f2.x - f1.x;
                const auto dy = f2.y - f1.y;
                const auto distance = std::sqrt(dx * dx + dy * dy);

                const auto delta = distance - m_lastZoomDelta;
                if (std::abs(delta) > ZOOM_THRESHOLD) {
                    const auto centerX = (f1.x + f2.x) * 0.5f * m_displaySize.m_x;
                    const auto centerY = (f1.y + f2.y) * 0.5f * m_displaySize.m_y;

                    onCameraZoom(delta, centerX, centerY);

                    m_lastZoomDelta = distance;
                }
            }

            break;
    }
}

void GraphViewModel::preRenderUpdate() { selectNodesInBox(); }

std::vector<VisibleNode>& GraphViewModel::getVisibleNodes() {
    if (m_lastQueryRegionArea.contains(m_visibleRegionArea)) {
        return m_visibleNodes;
    }

    const auto extraMargin = m_displaySize * 0.2f;
    m_lastQueryRegionArea = {
        screenToWorld(-extraMargin),
        screenToWorld(m_displaySize + extraMargin),
    };

    int maxNodesPerCell = -1;
    if (m_camera.m_zoom <= m_nodeCondensationFactor && m_shouldCondensateNodesLowZoom) {
        maxNodesPerCell = static_cast<int>(m_maxNodesPerCellBase / m_camera.m_zoom);
    }

    m_visibleNodes.clear();
    m_model->visitNodes(
        m_lastQueryRegionArea,
        [this](NodeIndex_t nodeIndex) {
            const auto node = m_model->getNode(nodeIndex);
            m_visibleNodes.emplace_back(node->getWorldPos(), nodeIndex);
        },
        maxNodesPerCell);

    return m_visibleNodes;
}

NodeIndex_t GraphViewModel::getHoveredNodeIndex() const { return m_hoveredNodeIndex; }

size_t GraphViewModel::getSelectedNodesCount() const { return m_selectedNodes.size(); }

bool GraphViewModel::isNodeSelected(NodeIndex_t nodeIndex) const {
    return m_model->getNode(nodeIndex)->isSelected();
}

float GraphViewModel::getZoomFactor() const { return m_camera.m_zoom; }

void GraphViewModel::setZoomFactor(float zoom) {
    m_camera.m_zoom = zoom;

    clampCameraPositionInBounds();
    invalidateVisibleNodesCache();
    updateVisibleRegion();
}

int GraphViewModel::getMaxNodesPerCellBase() const { return m_maxNodesPerCellBase; }

void GraphViewModel::setMaxNodesPerCellBase(int maxNodes) {
    m_maxNodesPerCellBase = maxNodes;
    invalidateVisibleNodesCache();
}

float GraphViewModel::getNodeCondensationFactor() const { return m_nodeCondensationFactor; }

void GraphViewModel::setNodeCondensationFactor(float factor) {
    m_nodeCondensationFactor = factor;
    invalidateVisibleNodesCache();
}

bool GraphViewModel::shouldCondensateNodesLowZoom() const { return m_shouldCondensateNodesLowZoom; }

void GraphViewModel::setShouldCondensateNodesLowZoom(bool shouldCondensate) {
    m_shouldCondensateNodesLowZoom = shouldCondensate;
    invalidateVisibleNodesCache();
}

Vector2D GraphViewModel::getCameraPosition() const { return m_camera.m_position; }

BoundingBox2D GraphViewModel::getVisibleRegionWorld(Vector2D additionalPadding) const {
    BoundingBox2D visibleWorldBounds = {screenToWorld(-additionalPadding),
                                        screenToWorld(m_displaySize + additionalPadding)};
    const auto& graphBounds = m_model->getGraphBounds();
    return visibleWorldBounds.clamp(graphBounds);
}

Vector2D GraphViewModel::worldToScreen(Vector2D worldPos) const {
    return (worldPos - m_camera.m_position) * m_camera.m_zoom + m_displaySize * 0.5f;
}

Vector2D GraphViewModel::screenToWorld(Vector2D screenPos) const {
    return (screenPos - m_displaySize * 0.5f) / m_camera.m_zoom + m_camera.m_position;
}

void GraphViewModel::removeSelectedNodes() {
    m_model->removeSelectedNodes();

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

    const auto nodeWorldPos = node->getWorldPos();
    const auto nodeScreenPos = worldToScreen(nodeWorldPos);

    m_camera.m_position = nodeWorldPos;
    m_hoveredNodeIndex = nodeIndex;

    updateVisibleRegion();
}

void GraphViewModel::onSceneResize(float displayWidth, float displayHeight) {
    m_displaySize = {displayWidth, displayHeight};

    invalidateVisibleNodesCache();
    updateVisibleRegion();
}

void GraphViewModel::onCameraPan(float deltaX, float deltaY) {
    m_camera.m_position.m_x += deltaX / m_camera.m_zoom;
    m_camera.m_position.m_y += deltaY / m_camera.m_zoom;

    clampCameraPositionInBounds();
    updateVisibleRegion();
}

void GraphViewModel::onCameraZoom(float deltaZoom, float cursorX, float cursorY) {
    const auto world = screenToWorld({cursorX, cursorY});

    m_camera.m_zoom += (deltaZoom > 0) ? 0.1f : -0.1f;
    m_camera.m_zoom = std::clamp(m_camera.m_zoom, 0.1f, 5.f);

    m_camera.m_position.m_x = world.m_x - (cursorX - m_displaySize.m_x * 0.5f) / m_camera.m_zoom;
    m_camera.m_position.m_y = world.m_y - (cursorY - m_displaySize.m_y * 0.5f) / m_camera.m_zoom;

    clampCameraPositionInBounds();
    invalidateVisibleNodesCache();
    updateVisibleRegion();
}

void GraphViewModel::onMouseClick(float cursorX, float cursorY, bool ctrlPressed) {
    if (m_hoveredNodeIndex != INVALID_NODE) {
        if (ctrlPressed) {
            if (isNodeSelected(m_hoveredNodeIndex)) {
                deselectNode(m_hoveredNodeIndex);
            } else {
                selectNode(m_hoveredNodeIndex);
            }
        } else {
            deselectAllNodes();
            selectNode(m_hoveredNodeIndex);
        }

        return;
    }

    if (!m_selectedNodes.empty()) {
        deselectAllNodes();
        return;
    }

    const auto worldPos = screenToWorld({cursorX, cursorY});
    const auto nodeArea = Node::getBoundingBox(worldPos);

    if (!WORLD_BOUNDS.contains(nodeArea)) {
        return;
    }

    if (m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}), true, NODE_DIAMETER)) {
        return;
    }

    m_model->addNode(worldPos);

    const auto lastNodeIndex = m_model->getLastNodeIndex();
    const auto lastNode = m_model->getNode(lastNodeIndex);

    m_visibleNodes.emplace_back(lastNode->getWorldPos(), lastNodeIndex);
    m_hoveredNodeIndex = lastNodeIndex;
}

void GraphViewModel::onMouseMove(float cursorX, float cursorY) {
    const auto hoveredNode = m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}));
    if (!hoveredNode) {
        m_hoveredNodeIndex = INVALID_NODE;
        return;
    }

    m_hoveredNodeIndex = m_model->getNodeIndex(hoveredNode);
}

void GraphViewModel::setSelectBoxStart(float cursorX, float cursorY) {
    m_selectBoxStartWorldPos = screenToWorld({cursorX, cursorY});
    m_selectBoxBounds = {m_selectBoxStartWorldPos, m_selectBoxStartWorldPos};
}

void GraphViewModel::setSelectBoxEnd(float cursorX, float cursorY) {
    const auto worldPos = screenToWorld({cursorX, cursorY});
    m_selectBoxBounds = BoundingBox2D{Vector2D::min(m_selectBoxStartWorldPos, worldPos),
                                      Vector2D::max(m_selectBoxStartWorldPos, worldPos)};
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

    constexpr auto MAX_SELECTABLE_NODES = 5'000'000;
    m_model->visitNodes(m_selectBoxBounds, [this](NodeIndex_t nodeIndex) {
        if (m_selectedNodes.size() >= MAX_SELECTABLE_NODES) {
            return false;
        }

        if (!m_model->getNode(nodeIndex)->isSelected()) {
            selectNode(nodeIndex);
        }

        return true;
    });

    m_lastSelectBoxQueryTime = now;
}

void GraphViewModel::selectNode(NodeIndex_t nodeIndex) {
    m_model->getNode(nodeIndex)->markSelected();
    m_selectedNodes.insert(nodeIndex);
}

void GraphViewModel::deselectNode(NodeIndex_t nodeIndex) {
    m_model->getNode(nodeIndex)->unmarkSelected();
    m_selectedNodes.erase(nodeIndex);
}

void GraphViewModel::deselectAllNodes() {
    for (const auto nodeIndex : m_selectedNodes) {
        m_model->getNode(nodeIndex)->unmarkSelected();
    }
    m_selectedNodes.clear();
}

void GraphViewModel::clampCameraPositionInBounds() {
    if (!WORLD_BOUNDS.contains(m_camera.m_position)) {
        m_camera.m_position.m_x =
            std::clamp(m_camera.m_position.m_x, WORLD_BOUNDS.m_min.m_x, WORLD_BOUNDS.m_max.m_x);
        m_camera.m_position.m_y =
            std::clamp(m_camera.m_position.m_y, WORLD_BOUNDS.m_min.m_y, WORLD_BOUNDS.m_max.m_y);
    }
}

void GraphViewModel::updateVisibleRegion() {
    m_visibleRegionArea = {screenToWorld({0.f, 0.f}), screenToWorld(m_displaySize)};
}

void GraphViewModel::invalidateVisibleNodesCache() {
    m_visibleNodes.clear();
    m_lastQueryRegionArea = {};
}

void GraphViewModel::addSampleNodes() {
    constexpr float start = -10000.f;
    constexpr float end = -start;
    constexpr float step = NODE_RADIUS * 2.f;

    constexpr size_t stepsPerAxis = static_cast<size_t>((end - start) / step) + 1;
    constexpr size_t nodeCount = stepsPerAxis * stepsPerAxis;

    static_assert(nodeCount < NODE_LIMIT, "Node count exceeds limits");

    m_model->beginBulkInsert();

    m_model->reserveNodes(nodeCount);
    m_model->reserveArea({{start, start}, {end, end}});

    std::cout << "Adding " << nodeCount << " nodes for testing...\n";

    for (float y = start; y <= end; y += step) {
        bool nodeLimitReached = false;
        if (nodeLimitReached) {
            std::cout << "Node limit reached while adding sample nodes.\n";
            break;
        }

        for (float x = start; x <= end; x += step) {
            const auto lastNodeIndex = m_model->getLastNodeIndex();
            if (lastNodeIndex != INVALID_NODE && lastNodeIndex >= nodeCount - 1) {
                nodeLimitReached = true;
                break;
            }

            m_model->addNode({x, y});
        }
    }

    std::cout << "Building GridMap.\n";

    const auto now = std::chrono::steady_clock::now();
    m_model->endBulkInsert();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - now);

    std::cout << "GridMap built in " << duration.count() << " ms.\n";
    std::cout << "Finished adding nodes.\n";
}

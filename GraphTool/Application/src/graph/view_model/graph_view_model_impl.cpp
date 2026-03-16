module;
#include <pch.h>

module graph_view_model;

import graph_loader;

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

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            m_shouldBlockMouseLeftClick = true;
        }

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
                case SDLK_F5:
                    refreshVisibleData();
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

void GraphViewModel::preRenderUpdate() {
    selectNodesInBox();

    const auto lastWidth = m_lastQueryRegionArea.width();
    const auto lastHeight = m_lastQueryRegionArea.height();

    const auto visibleWidth = m_visibleRegionArea.width();
    const auto visibleHeight = m_visibleRegionArea.height();

    constexpr auto alpha = 0.1f;
    const auto smallerLastQueryRegion =
        visibleWidth < lastWidth * alpha || visibleHeight < lastHeight * alpha;

    if (!m_lastQueryRegionArea.contains(m_visibleRegionArea) || smallerLastQueryRegion) {
        invalidateVisibleData();
        updateVisibleRegion();

        const auto extraMargin = m_displaySize * 1.25f;
        m_lastQueryRegionArea = {
            screenToWorld(-extraMargin),
            screenToWorld(m_displaySize + extraMargin),
        };

        m_lastQueryRegionArea.clamp(m_model->getGraphBounds());

        updateVisibleNodes(m_visibleData);
        updateVisibleEdges(m_visibleData);

        m_shouldUseCachedVisibleNodes = false;

        for (auto* listener : m_listeners) {
            listener->onFullDataUpdate();
        }
    }
}

void GraphViewModel::addListener(IGraphViewModelListener* listener) {
    m_listeners.push_back(listener);
}

const std::vector<Vector2D>& GraphViewModel::getVisibleNodesPositions() const {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_nodesPositions;
    }

    return m_visibleData.m_nodesPositions;
}

std::vector<NodeColorInfo>& GraphViewModel::getVisibleNodesColors() {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_nodesColors;
    }

    return m_visibleData.m_nodesColors;
}

const std::vector<NodeIndex_t>& GraphViewModel::getVisibleNodesIndexes() const {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_nodesIndexes;
    }

    return m_visibleData.m_nodesIndexes;
}

const std::vector<VisibleNode>& GraphViewModel::getVisibleNodes() const {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_visibleNodes;
    }

    return m_visibleData.m_visibleNodes;
}

const std::vector<VisibleEdge>& GraphViewModel::getVisibleEdges() const {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_visibleEdges;
    }

    return m_visibleData.m_visibleEdges;
}

void GraphViewModel::refreshVisibleData() { invalidateVisibleData(); }

NodeIndex_t GraphViewModel::getHoveredNodeIndex() const { return m_hoveredNodeIndex; }

size_t GraphViewModel::getSelectedNodesCount() const { return m_selectedNodes.size(); }

bool GraphViewModel::isNodeSelected(NodeIndex_t nodeIndex) const {
    return m_model->getNode(nodeIndex)->isSelected();
}

bool GraphViewModel::isValidLookupIndex(NodeIndex_t nodeIndex, uint32_t lookupIndex) const {
    if (lookupIndex >= getVisibleNodes().size()) {
        return false;
    }

    if (nodeIndex != getVisibleNodesIndexes()[lookupIndex]) {
        return false;
    }

    return true;
}

float GraphViewModel::getZoomFactor() const { return m_camera.m_zoom; }

void GraphViewModel::setZoomFactor(float zoom) {
    const auto oldZoom = m_camera.m_zoom;

    m_camera.m_zoom = zoom;

    clampCameraPositionInBounds();

    const auto hadCondensatedEarlier = oldZoom <= m_nodeCondensationFactor;
    const auto hasCondensatedNow = m_camera.m_zoom <= m_nodeCondensationFactor;

    if (m_shouldCondensateNodesLowZoom && hadCondensatedEarlier != hasCondensatedNow) {
        invalidateVisibleData();
    }

    updateVisibleRegion();
}

int GraphViewModel::getMaxNodesPerCellBase() const { return m_maxNodesPerCellPercentage; }

void GraphViewModel::setMaxNodesPerCellBase(int maxNodes) {
    m_maxNodesPerCellPercentage = maxNodes;
    invalidateVisibleData();
}

float GraphViewModel::getNodeCondensationPercentage() const { return m_nodeCondensationFactor; }

void GraphViewModel::setNodeCondensationPercentage(float percentage) {
    m_nodeCondensationFactor = percentage;
    invalidateVisibleData();
}

int GraphViewModel::getEdgeDrawPercentage() const { return m_edgeDrawPercentage; }

void GraphViewModel::setEdgeDrawPercentage(int percentage) {
    m_edgeDrawPercentage = percentage;
    invalidateVisibleData();
}

int GraphViewModel::getMaxVisibleNodes() const { return m_maxVisibleNodes; }

void GraphViewModel::setMaxVisibleNodes(int maxNodes) {
    m_maxVisibleNodes = maxNodes;
    invalidateVisibleData();
}

float GraphViewModel::getNodesRadius() const { return m_nodesRadius; }

void GraphViewModel::setNodesRadius(float radius) { m_nodesRadius = radius; }

bool GraphViewModel::shouldCondensateNodesLowZoom() const { return m_shouldCondensateNodesLowZoom; }

void GraphViewModel::setShouldCondensateNodesLowZoom(bool shouldCondensate) {
    m_shouldCondensateNodesLowZoom = shouldCondensate;
    invalidateVisibleData();
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

    m_visibleData = {};
    m_lastQueryRegionArea = {};

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
    const auto oldZoom = m_camera.m_zoom;

    if (deltaZoom > 0 && m_camera.m_zoom < 0.1f) {
        m_camera.m_zoom = 0.1f;
    } else {
        m_camera.m_zoom += (deltaZoom > 0) ? 0.1f : -0.1f;
        m_camera.m_zoom = std::clamp(m_camera.m_zoom, 0.05f, 50.f);
    }

    m_camera.m_position.m_x = world.m_x - (cursorX - m_displaySize.m_x * 0.5f) / m_camera.m_zoom;
    m_camera.m_position.m_y = world.m_y - (cursorY - m_displaySize.m_y * 0.5f) / m_camera.m_zoom;

    clampCameraPositionInBounds();

    const auto hadCondensatedEarlier = oldZoom <= m_nodeCondensationFactor;
    const auto hasCondensatedNow = m_camera.m_zoom <= m_nodeCondensationFactor;

    if (m_shouldCondensateNodesLowZoom && hadCondensatedEarlier != hasCondensatedNow) {
        invalidateVisibleData();
    }

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
    if (!WORLD_BOUNDS.contains(worldPos)) {
        return;
    }

    if (m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}), m_nodesRadius * 2.f, true)) {
        return;
    }

    m_model->addNode(worldPos);

    const auto lastNodeIndex = m_model->getLastNodeIndex();
    const auto lastNode = m_model->getNode(lastNodeIndex);

    onVisibleNode(lastNodeIndex, m_visibleData);

    NodeIndex_t oldHoveredNodeIndex = m_hoveredNodeIndex;
    m_hoveredNodeIndex = lastNodeIndex;

    for (auto* listener : m_listeners) {
        if (oldHoveredNodeIndex != INVALID_NODE) {
            listener->onNodeUnhover(oldHoveredNodeIndex);
        }

        listener->onNodeAdded(lastNodeIndex);
        listener->onNodeHover(lastNodeIndex);
    }
}

void GraphViewModel::onMouseMove(float cursorX, float cursorY) {
    const auto hoveredNode =
        m_model->getNodeAtPosition(screenToWorld({cursorX, cursorY}), m_nodesRadius);
    if (!hoveredNode) {
        NodeIndex_t oldHoveredNodeIndex = m_hoveredNodeIndex;
        m_hoveredNodeIndex = INVALID_NODE;

        if (oldHoveredNodeIndex != INVALID_NODE) {
            for (auto* listener : m_listeners) {
                listener->onNodeUnhover(oldHoveredNodeIndex);
            }
        }

        return;
    }

    NodeIndex_t oldHoveredNodeIndex = m_hoveredNodeIndex;
    m_hoveredNodeIndex = m_model->getNodeIndex(hoveredNode);

    for (auto* listener : m_listeners) {
        if (oldHoveredNodeIndex != INVALID_NODE) {
            listener->onNodeUnhover(oldHoveredNodeIndex);
        }

        listener->onNodeHover(m_hoveredNodeIndex);
    }
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
    m_model->visitNodes(
        m_selectBoxBounds,
        [this](NodeIndex_t nodeIndex) {
            if (m_selectedNodes.size() >= MAX_SELECTABLE_NODES) {
                return false;
            }

            if (!m_model->getNode(nodeIndex)->isSelected()) {
                selectNode(nodeIndex);
            }

            return true;
        },
        m_nodesRadius);

    m_lastSelectBoxQueryTime = now;
}

void GraphViewModel::selectNode(NodeIndex_t nodeIndex) {
    m_model->getNode(nodeIndex)->markSelected();
    m_selectedNodes.insert(nodeIndex);

    for (auto* listener : m_listeners) {
        listener->onNodeSelected(nodeIndex);
    }
}

void GraphViewModel::deselectNode(NodeIndex_t nodeIndex) {
    m_model->getNode(nodeIndex)->unmarkSelected();
    m_selectedNodes.erase(nodeIndex);

    for (auto* listener : m_listeners) {
        listener->onNodeDeselected(nodeIndex);
    }
}

void GraphViewModel::deselectAllNodes() {
    for (const auto nodeIndex : m_selectedNodes) {
        m_model->getNode(nodeIndex)->unmarkSelected();

        for (auto* listener : m_listeners) {
            listener->onNodeDeselected(nodeIndex);
        }
    }
    m_selectedNodes.clear();
}

void GraphViewModel::updateVisibleNodes(VisibleData& visibleData) {
    const auto estimatedNodeCount = m_model->estimateNodeCountInArea(m_lastQueryRegionArea);

    const auto zoom = m_camera.m_zoom;
    float nodesPerCellPercentage = 1.f;
    if (m_shouldCondensateNodesLowZoom && zoom <= m_nodeCondensationFactor) {
        nodesPerCellPercentage = m_maxNodesPerCellPercentage / 100.f;
    }

    visibleData.m_nodesPositions.reserve(estimatedNodeCount);
    visibleData.m_nodesColors.reserve(estimatedNodeCount);
    visibleData.m_nodesIndexes.reserve(estimatedNodeCount);
    visibleData.m_visibleNodes.reserve(estimatedNodeCount);

    m_model->visitNodes(
        m_lastQueryRegionArea,
        [this, &visibleData](NodeIndex_t nodeIndex) {
            if (visibleData.m_visibleNodes.size() >= m_maxVisibleNodes) {
                return false;
            }

            onVisibleNode(nodeIndex, visibleData);
            return true;
        },
        m_nodesRadius, nodesPerCellPercentage);
}

void GraphViewModel::updateVisibleEdges(VisibleData& visibleData) {
#ifdef __EMSCRIPTEN__
    for (const auto [lookupIndex] : visibleData.m_visibleNodes) {
        const auto nodeIndex = visibleData.m_nodesIndexes[lookupIndex];

        struct VisitorData {
            VisibleData* visibleData;
            GraphModel* model;
            uint32_t srcLookupIndex;
        } visitorData{&visibleData, m_model, lookupIndex};

        m_model->visitNeighbours(
            nodeIndex, &visitorData,
            [](void* data, NodeIndex_t index, int) {
                const auto visitorData = static_cast<VisitorData*>(data);
                const auto visibleData = visitorData->visibleData;

                if (visibleData->m_visibleEdges.size() >= std::numeric_limits<int>::max() - 1) {
                    return false;
                }

                const auto destNode = visitorData->model->getNode(index);
                const auto destLookupIndex = destNode->getLookupIndex();

                if (destLookupIndex >= visibleData->m_visibleNodes.size()) {
                    return true;
                }

                if (index != visibleData->m_nodesIndexes[destLookupIndex]) {
                    return true;
                }

                visibleData->m_visibleEdges.emplace_back(visitorData->srcLookupIndex,
                                                         destLookupIndex);
                return true;
            },
            m_edgeDrawPercentage / 100.f);
    }
#else
    const auto threadCount = std::thread::hardware_concurrency();
    const auto totalNodes = visibleData.m_visibleNodes.size();
    if (totalNodes == 0) {
        return;
    }

    const auto chunkSize = (totalNodes + threadCount - 1) / threadCount;
    std::vector<std::vector<VisibleEdge> > threadEdges(threadCount);

    auto worker = [this, &visibleData, &threadEdges](size_t start, size_t end, size_t threadIndex) {
        for (size_t i = start; i < end; ++i) {
            const auto lookupIndex = visibleData.m_visibleNodes[i].m_lookupIndex;
            const auto nodeIndex = visibleData.m_nodesIndexes[lookupIndex];

            struct VisitorData {
                VisibleData* visibleData;
                GraphModel* model;
                uint32_t srcLookupIndex;
                std::vector<VisibleEdge>* edges;
            } visitorData{&visibleData, m_model, lookupIndex, &threadEdges[threadIndex]};

            m_model->visitNeighbours(
                nodeIndex, &visitorData,
                [](void* data, NodeIndex_t index, int) {
                    const auto visitorData = static_cast<VisitorData*>(data);
                    const auto visibleData = visitorData->visibleData;

                    if (visibleData->m_visibleEdges.size() >= std::numeric_limits<int>::max() - 1) {
                        return false;
                    }

                    const auto destNode = visitorData->model->getNode(index);
                    const auto destLookupIndex = destNode->getLookupIndex();

                    if (destLookupIndex >= visibleData->m_visibleNodes.size()) {
                        return true;
                    }

                    if (index != visibleData->m_nodesIndexes[destLookupIndex]) {
                        return true;
                    }

                    visitorData->edges->emplace_back(visitorData->srcLookupIndex, destLookupIndex);
                    return true;
                },
                m_edgeDrawPercentage / 100.f);
        }
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < threadCount; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, totalNodes);
        if (start >= end) {
            break;
        }

        threads.emplace_back(worker, start, end, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    size_t totalEdges = 0;
    for (const auto& edges : threadEdges) {
        totalEdges += edges.size();
    }

    visibleData.m_visibleEdges.reserve(totalEdges);
    for (const auto& edges : threadEdges) {
        visibleData.m_visibleEdges.insert(visibleData.m_visibleEdges.end(), edges.begin(),
                                          edges.end());
    }
#endif
}

void GraphViewModel::onVisibleNode(NodeIndex_t nodeIndex, VisibleData& visibleData) {
    const auto node = m_model->getNode(nodeIndex);

    visibleData.m_nodesPositions.push_back(node->getWorldPos());
    visibleData.m_nodesColors.emplace_back();
    visibleData.m_nodesIndexes.push_back(nodeIndex);

    const auto lookupIndex = static_cast<uint32_t>(visibleData.m_visibleNodes.size());
    node->setLookupIndex(lookupIndex);

    visibleData.m_visibleNodes.emplace_back(lookupIndex);

    for (auto* listener : m_listeners) {
        listener->onNodeAddedToVisibleData(nodeIndex, visibleData);
    }
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
    m_visibleRegionArea.clamp(m_model->getGraphBounds());
}

void GraphViewModel::invalidateVisibleData() {
    if (m_shouldUseCachedVisibleNodes) {
        return;
    }

    m_shouldUseCachedVisibleNodes = true;
    m_cachedVisibleData = std::move(m_visibleData);

    m_lastQueryRegionArea = {};
}

void GraphViewModel::addSampleNodes() {
    GraphLoader::loadJSON(m_model, R"(assets/brasov.json)");
    centerOnNode(0);

    return;

    constexpr float start = -500'000.f;
    constexpr float end = -start;
    const float step = m_nodesRadius * 1.f;

    const auto stepsPerAxis = static_cast<uint32_t>((end - start) / step) + 1;
    const auto nodeCount = std::clamp(stepsPerAxis * stepsPerAxis, 1u, (uint32_t)NODE_LIMIT);

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

    size_t added = 0;
    while (added < 5'000'000) {
        NodeIndex_t src = xorshift32() % nodeCount;
        NodeIndex_t dest = xorshift32() % nodeCount;

        if (src == dest) continue;
        if (src > dest) std::swap(src, dest);

        m_model->addEdge(src, dest, 1);
        added++;
    }
}

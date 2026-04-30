module;
#include <pch.h>

module graph_view_model;

import graph_loader;
import graph_common;

import algorithm_factory;

GraphViewModel::GraphViewModel(GraphViewModel&& rhs) noexcept
    : m_camera(rhs.m_camera),
      m_displaySize(rhs.m_displaySize),
      m_selectedNodes(std::move(rhs.m_selectedNodes)),
      m_maxNodesPerCellPercentage(rhs.m_maxNodesPerCellPercentage),
      m_nodeCondensationFactor(rhs.m_nodeCondensationFactor),
      m_edgeDrawPercentage(rhs.m_edgeDrawPercentage),
      m_maxVisibleNodes(rhs.m_maxVisibleNodes),
      m_nodesRadius(rhs.m_nodesRadius),
      m_shouldCondensateNodesLowZoom(rhs.m_shouldCondensateNodesLowZoom),
      m_listeners(std::move(rhs.m_listeners)) {
    m_visibleData = m_cachedVisibleData = {};
}

GraphViewModel& GraphViewModel::operator=(GraphViewModel&& rhs) noexcept {
    if (this != &rhs) {
        m_camera = rhs.m_camera;
        m_displaySize = rhs.m_displaySize;
        m_selectedNodes = std::move(rhs.m_selectedNodes);
        m_maxNodesPerCellPercentage = rhs.m_maxNodesPerCellPercentage;
        m_nodeCondensationFactor = rhs.m_nodeCondensationFactor;
        m_edgeDrawPercentage = rhs.m_edgeDrawPercentage;
        m_maxVisibleNodes = rhs.m_maxVisibleNodes;
        m_nodesRadius = rhs.m_nodesRadius;
        m_shouldCondensateNodesLowZoom = rhs.m_shouldCondensateNodesLowZoom;
        m_listeners = std::move(rhs.m_listeners);

        m_visibleData = m_cachedVisibleData = {};
    }

    return *this;
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
            } else {
                onMouseMove(event.motion.x, event.motion.y);
            }

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
    tickAlgorithmExecution();

    const auto lastWidth = m_lastQueryRegionArea.width();
    const auto lastHeight = m_lastQueryRegionArea.height();

    const auto visibleWidth = m_visibleRegionArea.width();
    const auto visibleHeight = m_visibleRegionArea.height();

    constexpr auto alpha = 0.1f;
    const auto smallerLastQueryRegion =
        visibleWidth < lastWidth * alpha || visibleHeight < lastHeight * alpha;

    if (!m_isUpdateFutureRunning &&
        (!m_lastQueryRegionArea.contains(m_visibleRegionArea) || m_lastQueryRegionArea.null() ||
         smallerLastQueryRegion || m_shouldUseCachedVisibleNodes)) {
        invalidateVisibleData();
        updateVisibleRegion();

        const auto extraMargin = m_displaySize * m_overscanFactor;
        m_lastQueryRegionArea = {
            screenToWorld(-extraMargin),
            screenToWorld(m_displaySize + extraMargin),
        };

        m_lastQueryRegionArea.clamp(WORLD_BOUNDS);

#ifdef __EMSCRIPTEN__
        common::ScopedTimer timer("Updating visible data");

        updateVisibleNodes(m_visibleData);
        updateVisibleEdges(m_visibleData);

        for (auto* listener : m_listeners) {
            listener->onFullDataUpdate();
        }

        m_shouldUseCachedVisibleNodes = false;
#else
        m_isUpdateFutureRunning = true;
        m_updateFuture = std::async(std::launch::async, [this]() {
            common::ScopedTimer timer("Updating visible data");

            VisibleData newVisibleData;
            updateVisibleNodes(newVisibleData);
            updateVisibleEdges(newVisibleData);
            return newVisibleData;
        });
#endif
    }

#ifndef __EMSCRIPTEN__
    if (m_isUpdateFutureRunning && m_updateFuture.valid() &&
        m_updateFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        m_visibleData = std::move(m_updateFuture.get());

        for (auto* listener : m_listeners) {
            listener->onFullDataUpdate();
        }

        m_shouldUseCachedVisibleNodes = false;
        m_isUpdateFutureRunning = false;
    }
#endif
}

void GraphViewModel::setModel(GraphModel* model) { m_model = model; }

GraphModel* GraphViewModel::getModel() const { return m_model; }

void GraphViewModel::addListener(IGraphViewModelListener* listener) {
    m_listeners.push_back(listener);
}

Vector2D GraphViewModel::getSceneSize() const { return m_displaySize; }

void GraphViewModel::updateSceneSize(float displayWidth, float displayHeight) {
    onSceneResize(displayWidth, displayHeight);
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

const std::vector<NodeIndex_t>& GraphViewModel::getVisibleNodes() const {
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

const std::vector<uint8_t>& GraphViewModel::getVisibleLoops() const {
    if (m_shouldUseCachedVisibleNodes) {
        return m_cachedVisibleData.m_visibleLoops;
    }

    return m_visibleData.m_visibleLoops;
}

void GraphViewModel::refreshVisibleData() { invalidateVisibleData(); }

NodeIndex_t GraphViewModel::getHoveredNodeIndex() const { return m_hoveredNodeIndex; }

std::pair<NodeIndex_t, NodeIndex_t> GraphViewModel::getSelectedNodesPair() const {
    NodeIndex_t first = INVALID_NODE, second = INVALID_NODE;

    if (!m_selectedNodes.empty()) {
        auto it = m_selectedNodes.begin();
        first = *it;
        it = std::next(it);
        if (it != m_selectedNodes.end()) {
            second = *it;
        }
    }

    return std::make_pair(first, second);
}

size_t GraphViewModel::getSelectedNodesCount() const { return m_selectedNodes.size(); }

bool GraphViewModel::isNodeSelected(NodeIndex_t nodeIndex) const {
    return m_model->getNode(nodeIndex)->isSelected();
}

std::optional<uint32_t> GraphViewModel::getLookupIndex(NodeIndex_t nodeIndex) const {
    const auto& visibleNodes = getVisibleNodes();

    auto it = std::lower_bound(visibleNodes.begin(), visibleNodes.end(), nodeIndex);
    if (it != visibleNodes.end() && *it == nodeIndex) {
        return static_cast<uint32_t>(std::distance(visibleNodes.begin(), it));
    }

    return std::nullopt;
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

float GraphViewModel::getOverscanFactor() const { return m_overscanFactor; }

void GraphViewModel::setOverscanFactor(float factor) {
    m_overscanFactor = factor;
    invalidateVisibleData();
}

Vector2D GraphViewModel::getCameraPosition() const { return m_camera.m_position; }

BoundingBox2D GraphViewModel::getVisibleRegionWorld(Vector2D additionalPadding) const {
    BoundingBox2D visibleWorldBounds = {screenToWorld(-additionalPadding),
                                        screenToWorld(m_displaySize + additionalPadding)};
    const auto& graphBounds = m_model->getGraphBounds();
    return visibleWorldBounds.clamp(WORLD_BOUNDS);
}

Vector2D GraphViewModel::worldToScreen(Vector2D worldPos) const {
    return (worldPos - m_camera.m_position) * m_camera.m_zoom + m_displaySize * 0.5f;
}

Vector2D GraphViewModel::screenToWorld(Vector2D screenPos) const {
    return (screenPos - m_displaySize * 0.5f) / m_camera.m_zoom + m_camera.m_position;
}

void GraphViewModel::removeSelectedNodes() {
    if (isRunningUpdate()) {
        return;
    }

    common::ScopedTimer timer("GraphViewModel::removeSelectedNodes()");

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

    const auto oldHoveredNodeIndex = m_hoveredNodeIndex;
    m_hoveredNodeIndex = nodeIndex;

    for (auto* listener : m_listeners) {
        if (oldHoveredNodeIndex != INVALID_NODE) {
            listener->onNodeUnhover(oldHoveredNodeIndex);
        }

        listener->onNodeHover(nodeIndex);
    }

    updateVisibleRegion();
}

bool GraphViewModel::isRunningUpdate() const { return m_isUpdateFutureRunning; }

void GraphViewModel::cancelRunningUpdate() {
#ifndef __EMSCRIPTEN__
    if (m_isUpdateFutureRunning && m_updateFuture.valid()) {
        m_updateFuture.wait();
        m_isUpdateFutureRunning = false;
    }
#endif
}

void GraphViewModel::addEdge(NodeIndex_t from, NodeIndex_t to, int weight) {
    m_model->addEdge(from, to, weight);
    invalidateVisibleData();
}

void GraphViewModel::removeEdge(NodeIndex_t from, NodeIndex_t to) {
    m_model->removeEdge(from, to);
    invalidateVisibleData();
}

int GraphViewModel::getAlgorithmStepDelayMs() const { return m_algorithmStepDelayMs; }

void GraphViewModel::setAlgorithmStepDelayMs(int delayMs) { m_algorithmStepDelayMs = delayMs; }

int GraphViewModel::getAlgorithmIterationsPerStep() const { return m_iterationsPerStep; }

void GraphViewModel::setAlgorithmIterationsPerStep(int iterations) {
    m_iterationsPerStep = static_cast<uint16_t>(iterations);
}

bool GraphViewModel::isAlgorithmCreated() const { return m_runningAlgorithm != nullptr; }

bool GraphViewModel::isAlgorithmRunning() const {
    return m_runningAlgorithm && !m_runningAlgorithm->isFinished() && !m_isAlgorithmPaused;
}

bool GraphViewModel::isAlgorithmFinished() const {
    return m_runningAlgorithm && m_runningAlgorithm->isFinished();
}

AlgorithmType GraphViewModel::getRunningAlgorithmType() const {
    return m_runningAlgorithm->getType();
}

IAlgorithm::ExecutionInfo_t GraphViewModel::getRunningAlgorithmExecutionInfo() const {
    return m_runningAlgorithm ? m_runningAlgorithm->getExecutionInfo()
                              : IAlgorithm::ExecutionInfo_t{};
}

void GraphViewModel::startAlgorithm(AlgorithmType algorithmType, NodeIndex_t sourceNode,
                                    NodeIndex_t targetNode) {
    m_isAlgorithmPaused = true;

    m_runningAlgorithm = AlgorithmFactory::createAlgorithm(algorithmType);

    m_runningAlgorithm->addListener(this);
    m_runningAlgorithm->setSourceNode(sourceNode);
    m_runningAlgorithm->setTargetNode(targetNode);
    m_runningAlgorithm->setModel(m_model);

    m_runningAlgorithm->initialize();

    for (auto* listener : m_listeners) {
        listener->onAlgorithmStarted();
    }
}

void GraphViewModel::stopAlgorithm() {
    m_runningAlgorithm.reset();

    for (auto* listener : m_listeners) {
        listener->onAlgorithmAborted();
    }
}

void GraphViewModel::toggleAlgorithmPause() { m_isAlgorithmPaused = !m_isAlgorithmPaused; }

void GraphViewModel::stepForwardAlgorithm() {
    for (uint16_t i = 0; i < m_iterationsPerStep; ++i) {
        if (m_runningAlgorithm->isFinished()) {
            break;
        }

        m_runningAlgorithm->step();
    }
}

void GraphViewModel::stepBackwardAlgorithm() { m_runningAlgorithm->undo(m_iterationsPerStep); }

void GraphViewModel::finishAlgorithm() {
    while (!m_runningAlgorithm->isFinished()) {
        m_runningAlgorithm->step();
    }
}

void GraphViewModel::restartAlgorithm() { m_runningAlgorithm->restart(); }

void GraphViewModel::onAlgorithmFinish() {
    common::Logger::get().information("Algorithm {} finished.", m_runningAlgorithm->getName());
}

void GraphViewModel::onNodeStateChange(NodeIndex_t nodeIndex, NodeState newState) {
    for (auto* listener : m_listeners) {
        listener->onNodeStateChange(nodeIndex, newState, m_runningAlgorithm->getType());
    }
}

void GraphViewModel::onAlgorithmPseudocodeEvent(const std::string_view event) {
    for (auto* listener : m_listeners) {
        listener->onAlgorithmPseudocodeEvent(event);
    }
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

    const float step = (m_camera.m_zoom + 1e-6f < 0.1f) ? 0.01f : 0.1f;
    m_camera.m_zoom += (deltaZoom > 0) ? step : -step;
    m_camera.m_zoom = std::clamp(m_camera.m_zoom, 0.01f, 50.f);

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

    if (isRunningUpdate()) {
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

    visibleData.m_visibleNodes.reserve(
        static_cast<uint32_t>(estimatedNodeCount * nodesPerCellPercentage) + 1);

    m_model->visitNodes(
        m_lastQueryRegionArea,
        [this, &visibleData](NodeIndex_t nodeIndex) {
            if (visibleData.m_visibleNodes.size() >= m_maxVisibleNodes) {
                return false;
            }

            visibleData.m_visibleNodes.emplace_back(nodeIndex);
            return true;
        },
        m_nodesRadius, nodesPerCellPercentage);

    setupVisibleNodes(visibleData);
}

void GraphViewModel::updateVisibleEdges(VisibleData& visibleData) {
#ifdef __EMSCRIPTEN__
    for (uint32_t lookupIndex = 0; lookupIndex < visibleData.m_visibleNodes.size(); ++lookupIndex) {
        const auto nodeIndex = visibleData.m_visibleNodes[lookupIndex];

        struct VisitorData {
            VisibleData* visibleData;
            NodeIndex_t srcNodeIndex;
            uint32_t srcLookupIndex;
        } visitorData{&visibleData, nodeIndex, lookupIndex};

        m_model->visitDistinctNeighbours(
            nodeIndex, &visitorData,
            [](void* data, NodeIndex_t index, int, bool bothWays) {
                const auto visitorData = static_cast<VisitorData*>(data);
                const auto visibleData = visitorData->visibleData;

                if (visibleData->m_visibleEdges.size() >= std::numeric_limits<int>::max() - 1) {
                    return false;
                }

                const auto beginIt = visibleData->m_visibleNodes.begin();
                const auto endIt = visibleData->m_visibleNodes.end();

                const auto srcNodeIndex = visitorData->srcNodeIndex;
                const auto srcLookupIndex = visitorData->srcLookupIndex;

                std::vector<NodeIndex_t>::iterator destLookupIt;
                if (index > srcNodeIndex) {
                    destLookupIt = std::lower_bound(beginIt + srcLookupIndex, endIt, index);
                } else {
                    destLookupIt = std::lower_bound(beginIt, beginIt + srcLookupIndex, index);
                }

                if (destLookupIt == endIt || *destLookupIt != index) {
                    return true;
                }

                const auto destLookupIndex = static_cast<uint32_t>(destLookupIt - beginIt);
                if (srcLookupIndex == destLookupIndex) {
                    const auto srcLookupInVector = srcLookupIndex / 8;
                    const auto srcBitInByte = srcLookupIndex % 8;

                    visibleData->m_visibleLoops[srcLookupInVector] |=
                        static_cast<uint8_t>(1u << srcBitInByte);
                } else {
                    visibleData->m_visibleEdges.emplace_back(visitorData->srcLookupIndex,
                                                             destLookupIndex, bothWays);
                }

                return true;
            },
            m_edgeDrawPercentage / 100.f);
    }
#else
    const auto threadCount = std::max(1u, std::thread::hardware_concurrency());
    const auto totalNodes = static_cast<uint32_t>(visibleData.m_visibleNodes.size());
    if (totalNodes == 0) {
        return;
    }

    const auto chunkSize = (totalNodes + threadCount - 1) / threadCount;
    std::vector<std::vector<VisibleEdge> > threadEdges(threadCount);

    auto worker = [this, &visibleData, &threadEdges](uint32_t start, uint32_t end,
                                                     uint32_t threadIndex) {
        for (uint32_t lookupIndex = start; lookupIndex < end; ++lookupIndex) {
            const auto nodeIndex = visibleData.m_visibleNodes[lookupIndex];

            struct VisitorData {
                VisibleData* visibleData;
                NodeIndex_t srcNodeIndex;
                uint32_t srcLookupIndex;
                std::vector<VisibleEdge>* edges;
            } visitorData{&visibleData, nodeIndex, lookupIndex, &threadEdges[threadIndex]};

            m_model->visitDistinctNeighbours(
                nodeIndex, &visitorData,
                [](void* data, NodeIndex_t index, int, bool bothWays) {
                    const auto visitorData = static_cast<VisitorData*>(data);
                    const auto visibleData = visitorData->visibleData;

                    if (visitorData->edges->size() >= std::numeric_limits<int>::max() - 1) {
                        return false;
                    }

                    const auto beginIt = visibleData->m_visibleNodes.begin();
                    const auto endIt = visibleData->m_visibleNodes.end();

                    const auto srcNodeIndex = visitorData->srcNodeIndex;
                    const auto srcLookupIndex = visitorData->srcLookupIndex;

                    std::vector<NodeIndex_t>::iterator destLookupIt;
                    if (index > srcNodeIndex) {
                        destLookupIt = std::lower_bound(beginIt + srcLookupIndex, endIt, index);
                    } else {
                        destLookupIt = std::lower_bound(beginIt, beginIt + srcLookupIndex, index);
                    }

                    if (destLookupIt == endIt || *destLookupIt != index) {
                        return true;
                    }

                    const auto destLookupIndex = static_cast<uint32_t>(destLookupIt - beginIt);
                    if (srcLookupIndex == destLookupIndex) {
                        const auto srcLookupInVector = srcLookupIndex / 8;
                        const auto srcBitInByte = srcLookupIndex % 8;

                        visibleData->m_visibleLoops[srcLookupInVector] |=
                            static_cast<uint8_t>(1u << srcBitInByte);
                    } else {
                        visitorData->edges->emplace_back(visitorData->srcLookupIndex,
                                                         destLookupIndex, bothWays);
                    }
                    return true;
                },
                m_edgeDrawPercentage / 100.f);
        }
    };

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < threadCount; ++i) {
        const auto start = i * chunkSize;
        const auto end = std::min(start + chunkSize, totalNodes);
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
    visibleData.m_nodesPositions.push_back(m_model->getNode(nodeIndex)->getWorldPos());
    visibleData.m_nodesColors.emplace_back();
    visibleData.m_visibleNodes.emplace_back(nodeIndex);
}

void GraphViewModel::setupVisibleNodes(VisibleData& visibleData) {
    const auto size = visibleData.m_visibleNodes.size();
    std::vector<NodeIndex_t> tmpIndexes(size);

    for (auto byte = 0u; byte < 4; ++byte) {
        const auto shift = byte * 8;

        uint32_t count[256]{}, total = 0;
        for (auto i = 0u; i < size; ++i) {
            const auto key = (visibleData.m_visibleNodes[i] >> shift) & 0xFF;
            ++count[key];
        }

        for (auto i = 0u; i < 256; ++i) {
            const auto oldCount = count[i];
            count[i] = total;
            total += oldCount;
        }

        for (auto i = 0u; i < size; ++i) {
            const auto key = (visibleData.m_visibleNodes[i] >> shift) & 0xFF;
            const auto index = count[key]++;

            tmpIndexes[index] = visibleData.m_visibleNodes[i];
        }

        visibleData.m_visibleNodes.swap(tmpIndexes);
    }

    visibleData.m_nodesPositions.reserve(size);
    visibleData.m_nodesColors.resize(size);

    const auto visibleLoopsSize = (size + 7) / 8;
    visibleData.m_visibleLoops.resize(visibleLoopsSize, 0);

    for (uint32_t lookupIndex = 0; lookupIndex < visibleData.m_visibleNodes.size(); ++lookupIndex) {
        const auto nodeIndex = visibleData.m_visibleNodes[lookupIndex];
        const auto node = m_model->getNode(nodeIndex);

        visibleData.m_nodesPositions.push_back(node->getWorldPos());
        for (auto* listener : m_listeners) {
            listener->onNodeAddedToVisibleData(nodeIndex, lookupIndex, visibleData);
        }
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
    m_visibleRegionArea.clamp(WORLD_BOUNDS);
}

void GraphViewModel::invalidateVisibleData() {
    if (m_shouldUseCachedVisibleNodes) {
        return;
    }

    m_shouldUseCachedVisibleNodes = true;
    m_cachedVisibleData = std::move(m_visibleData);

    m_lastQueryRegionArea = {};
}

void GraphViewModel::tickAlgorithmExecution() {
    if (!isAlgorithmRunning()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAlgorithmStepTime);

    if (duration.count() < m_algorithmStepDelayMs) {
        return;
    }

    for (uint16_t i = 0; i < m_iterationsPerStep; ++i) {
        if (m_runningAlgorithm->isFinished()) {
            break;
        }

        m_runningAlgorithm->step();
    }

    m_lastAlgorithmStepTime = now;
}

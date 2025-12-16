#include <pch.h>

#include "GraphManager.h"

#include "storage/AdjacencyList.h"
#include "storage/AdjacencyMatrix.h"

#include "../random/Random.h"

GraphManager::GraphManager() : m_graphStorage(std::make_unique<AdjacencyList>()) {
    setFlag(ItemIsFocusable);
    connect(&m_sceneUpdateTimer, &QTimer::timeout, [this]() {
        QGraphicsView* view = scene()->views().first();
        const QRect sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect().toRect();
        if (m_sceneRect != sceneRect) {
            m_sceneRect = sceneRect;
            update(m_sceneRect);
        }
    });

    connect(&m_edgeWatcher, &QFutureWatcher<QPainterPath>::finished, [this]() {
        if (!m_edgeFuture.isCanceled()) {
            if (m_loadingScreen) {
                m_loadingScreen->close();
            }

            m_edgesCache = m_edgeWatcher.result();
            update();
        }
    });
}

void GraphManager::setSceneDimensions(qreal width, qreal height) {
    m_boundingRect.setCoords(0, 0, width, height);
    m_quadTree.setBoundary(QRect(0, 0, width, height));

    m_sceneUpdateTimer.start(200);
}

bool GraphManager::isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore) const {
    constexpr auto r = NodeData::k_radius;
    if (!m_boundingRect.adjusted(r, r, -r, -r).contains(pos)) {
        return false;
    }

    if (!m_collisionsCheckEnabled) {
        return true;
    }

    return !m_quadTree.intersectsAnotherNode(pos, nodeToIgnore);
}

void GraphManager::setCollisionsCheckEnabled(bool enabled) { m_collisionsCheckEnabled = enabled; }

void GraphManager::reset() {
    m_nodes.clear();
    m_quadTree.clear();
    m_edgesCache.clear();
    m_selectedNodes.clear();

    resetAdjacencyMatrix();
}

size_t GraphManager::getNodesCount() const { return m_nodes.size(); }

void GraphManager::reserveNodes(size_t count) { m_nodes.reserve(count); }

NodeData& GraphManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> GraphManager::getNode(const QPoint& pos, float minDistance) {
    return m_quadTree.getNodeAtPosition(pos, minDistance);
}

bool GraphManager::hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const {
    return m_graphStorage->hasEdge(index, neighbour);
}

bool GraphManager::addNode(const QPoint& pos) {
    if (m_nodes.size() >= NODE_LIMIT || !isGoodPosition(pos)) {
        return false;
    }

    NodeData nodeData(m_nodes.size(), pos);
    nodeData.setFillColor(m_nodeDefaultColor);

    m_nodes.push_back(nodeData);
    m_quadTree.insert(nodeData);

    update(nodeData.getBoundingRect());
    return true;
}

void GraphManager::addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost) {
    constexpr auto numBits = sizeof(CostType_t) * 8 - 1;
    constexpr auto maxCost = (1 << (numBits - 1)) - 1;
    constexpr auto minCost = -(1 << (numBits - 1));

    switchToOptimalAdjacencyContainerIfNeeded();

    cost = std::clamp(cost, minCost, maxCost);
    m_graphStorage->addEdge(start, end, cost);
}

void GraphManager::randomlyAddEdges(size_t edgeCount) {}

size_t GraphManager::getMaxEdgesCount() const {
    const size_t n = m_nodes.size();
    if (m_orientedGraph) {
        return m_allowLoops ? n * n : n * (n - 1);
    }

    return n * (n - 1) / 2;
}

void GraphManager::resizeAdjacencyMatrix(size_t nodeCount) { m_graphStorage->resize(nodeCount); }

void GraphManager::resetAdjacencyMatrix() { m_graphStorage = std::make_unique<AdjacencyList>(); }

void GraphManager::buildVisibleEdgeCache() {
    if (!m_drawEdges) {
        return;
    }

    if (m_edgeFuture.isRunning()) {
        m_edgeFuture.cancel();
        m_edgeFuture.waitForFinished();
    }

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    m_edgeDensity.fill(0);
    m_edgesCache.clear();

    m_loadingScreen = new LoadingScreen("Building visible edge cache");
    m_loadingScreen->forceShow();

    m_edgeFuture = QtConcurrent::mappedReduced<QPainterPath>(
        visibleNodes,
        [&](NodeIndex_t nodeIndex) {
            QPainterPath edgePath;

            m_graphStorage->forEachOutgoingEdge(
                nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                    if (edgePath.elementCount() > SHOWN_EDGE_LIMIT) {
                        update(m_sceneRect);
                        return;
                    }

                    const auto srcCenter = m_nodes[nodeIndex].getPosition();
                    const auto targetCenter = m_nodes[neighbourIndex].getPosition();
                    if (!rasterizeEdgeAndCheckDensity(srcCenter, targetCenter)) {
                        return;
                    }

                    addEdgeToPath(edgePath, nodeIndex, neighbourIndex, cost);
                });

            return edgePath;
        },
        [](QPainterPath& result, const QPainterPath& edgePath) { result.addPath(edgePath); });

    m_edgeWatcher.setFuture(m_edgeFuture);
}

void GraphManager::buildFullEdgeCache() {
    const auto response = QMessageBox::information(nullptr, "Confirmation",
                                                   "Building the full edge cache may take a "
                                                   "long time for large graphs. Do you want "
                                                   "to proceed?",
                                                   QMessageBox::Yes, QMessageBox::No);
    if (response != QMessageBox::Yes) {
        return;
    }

    if (!m_drawEdges) {
        return;
    }

    if (m_edgeFuture.isRunning()) {
        m_edgeFuture.cancel();
        m_edgeFuture.waitForFinished();
    }

    m_edgeDensity.fill(0);
    m_edgesCache.clear();

    m_loadingScreen = new LoadingScreen("Building full edge cache");
    m_loadingScreen->forceShow();

    m_edgeFuture = QtConcurrent::mappedReduced<QPainterPath>(
        m_nodes,
        [&](const NodeData& nodeData) {
            NodeIndex_t nodeIndex = nodeData.getIndex();
            QPainterPath edgePath;

            m_graphStorage->forEachOutgoingEdge(
                nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                    addEdgeToPath(edgePath, nodeIndex, neighbourIndex, cost);
                });

            return edgePath;
        },
        [](QPainterPath& result, const QPainterPath& edgePath) { result.addPath(edgePath); });

    m_edgeWatcher.setFuture(m_edgeFuture);
}

void GraphManager::completeGraph() {
    if (m_nodes.empty()) {
        return;
    }

    auto newStorage = std::make_unique<AdjacencyMatrix>();
    newStorage->resize(m_nodes.size());
    newStorage->complete();
    m_graphStorage = std::move(newStorage);

    buildVisibleEdgeCache();
}

void GraphManager::fillGraph() {
    size_t attempts = 0;
    constexpr size_t maxAttempts = 10000;

    reset();
    reserveNodes(NODE_LIMIT);
    while (m_nodes.size() < NODE_LIMIT && attempts < maxAttempts) {
        QPoint pos(Random::get().getInt(0, m_boundingRect.width()),
                   Random::get().getInt(0, m_boundingRect.height()));

        if (isGoodPosition(pos)) {
            addNode(pos);
            attempts = 0;
        } else {
            ++attempts;
        }
    }

    resizeAdjacencyMatrix(m_nodes.size());
}

void GraphManager::setAllowEditing(bool enabled) { m_editingEnabled = enabled; }

bool GraphManager::getAllowEditing() const { return m_editingEnabled; }

void GraphManager::setAnimationsDisabled(bool disabled) { m_animationsDisabled = disabled; }

bool GraphManager::getAnimationsDisabled() const { return m_animationsDisabled; }

void GraphManager::setAllowLoops(bool allow) { m_allowLoops = allow; }

bool GraphManager::getAllowLoops() const { return m_allowLoops; }

void GraphManager::setOrientedGraph(bool oriented) { m_orientedGraph = oriented; }

bool GraphManager::getOrientedGraph() const { return m_orientedGraph; }

void GraphManager::setDrawNodesEnabled(bool enabled) { m_drawNodes = enabled; }

bool GraphManager::getDrawNodesEnabled() const { return m_drawNodes; }

void GraphManager::setDrawEdgesEnabled(bool enabled) { m_drawEdges = enabled; }

bool GraphManager::getDrawEdgesEnabled() const { return m_drawEdges; }

void GraphManager::setDrawQuadTreesEnabled(bool enabled) { m_drawQuadTrees = enabled; }

bool GraphManager::getDrawQuadTreesEnabled() const { return m_drawQuadTrees; }

void GraphManager::setNodeDefaultColor(QRgb color) {
    for (auto& node : m_nodes) {
        if (node.getFillColor() == m_nodeDefaultColor) {
            node.setFillColor(color);
            update(node.getBoundingRect());
        }
    }

    m_nodeDefaultColor = color;
}

void GraphManager::setNodeOutlineDefaultColor(QRgb color) { m_nodeOutlineDefaultColor = color; }

void GraphManager::dijkstra() {
    if (m_selectedNodes.size() != 2) {
        QMessageBox::warning(nullptr, "Dijkstra Error",
                             "Please select exactly two nodes to run Dijkstra's algorithm.");
        return;
    }

    m_algorithmPath.clear();

    std::vector<int32_t> minDistances(m_nodes.size(), std::numeric_limits<int32_t>::max());
    std::vector<NodeIndex_t> previousNodes(m_nodes.size(), -1);

    NodeIndex_t startNode = *m_selectedNodes.rbegin(), endNode = *m_selectedNodes.begin();
    if (m_nodes[startNode].getSelectTime() > m_nodes[endNode].getSelectTime()) {
        std::swap(startNode, endNode);
    }

    minDistances[startNode] = 0;

    using QueueElement = std::pair<int32_t, NodeIndex_t>;
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>>
        priorityQueue;
    priorityQueue.emplace(0, startNode);

    while (!priorityQueue.empty()) {
        const auto [currentDistance, currentNode] = priorityQueue.top();
        priorityQueue.pop();

        if (currentNode == endNode) {
            break;
        }

        if (currentDistance > minDistances[currentNode]) {
            continue;
        }

        m_graphStorage->forEachOutgoingEdgeWithOpposites(
            currentNode, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                if (cost < 0) {
                    throw std::runtime_error(
                        "Dijkstra's algorithm does not support graphs with negative edge weights.");
                }

                const auto newDistance = currentDistance + cost;
                if (newDistance < minDistances[neighbourIndex]) {
                    minDistances[neighbourIndex] = newDistance;
                    previousNodes[neighbourIndex] = currentNode;
                    priorityQueue.emplace(newDistance, neighbourIndex);
                }
            });
    }

    if (minDistances[endNode] == std::numeric_limits<int32_t>::max()) {
        QMessageBox::information(nullptr, "Dijkstra Result",
                                 "No path found between the selected nodes.");
        return;
    }

    uint64_t totalCost = 0;
    for (NodeIndex_t at = endNode; at != -1; at = previousNodes[at]) {
        const auto nextNode = previousNodes[at];
        if (nextNode != -1) {
            m_algorithmPath.moveTo(m_nodes[nextNode].getPosition());
            m_algorithmPath.lineTo(m_nodes[at].getPosition());
            totalCost += static_cast<uint64_t>(m_graphStorage->getCost(nextNode, at));
        }

        m_nodes[at].setFillColor(qRgb(255, 255, 0));
    }

    QMessageBox::information(nullptr, "Dijkstra Result",
                             QString("Path found with total cost: %1").arg(totalCost));
}

QRectF GraphManager::boundingRect() const { return m_boundingRect; }

void GraphManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    m_shouldDrawArrows = lod >= 1.25;
    drawEdgeCache(painter, lod);
    drawNodes(painter, lod);
    drawQuadTree(painter, &m_quadTree, lod);

    if (!m_algorithmPath.isEmpty()) {
        painter->setPen(QPen{Qt::yellow, 2.f});
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_algorithmPath);
    }
}

void GraphManager::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::AltModifier)) {
        setFlag(ItemIsSelectable);

        const auto point = event->pos().toPoint();
        const auto nodeOpt = getNode(point, (m_editingEnabled ? 1 : 4) * NodeData::k_radius);
        if (nodeOpt.has_value()) {
            if (!(event->modifiers() & Qt::ControlModifier)) {
                deselectNodes();
            }

            const auto nodeIndex = nodeOpt.value();
            if (m_nodes[nodeIndex].isSelected()) {
                m_nodes[nodeIndex].setSelected(false, 0);
                m_selectedNodes.erase(nodeIndex);
            } else {
                m_nodes[nodeIndex].setSelected(true, static_cast<uint32_t>(m_selectedNodes.size()));
                m_selectedNodes.emplace(nodeIndex);
            }

            m_dragOffset = getNode(nodeIndex).getPosition() - point;
        } else {
            if (!m_selectedNodes.empty() && !(event->modifiers() & Qt::ControlModifier)) {
                deselectNodes();
                m_pressedEmptySpace = false;
            } else {
                m_pressedEmptySpace = true;
            }
        }
    }

    QGraphicsObject::mousePressEvent(event);
}

void GraphManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && m_selectedNodes.size() == 1 &&
        !(event->modifiers() & Qt::ControlModifier)) {
        const auto selectedIndex = *m_selectedNodes.begin();
        auto& node = getNode(selectedIndex);
        const auto desiredPos = event->pos().toPoint() + m_dragOffset;

        if (isGoodPosition(desiredPos, selectedIndex)) {
            m_draggingNode = true;

            std::vector<QuadTree*> containingTrees;
            m_quadTree.getContainingTrees(node, containingTrees);

            const auto oldRect = node.getBoundingRect();
            node.setPosition(desiredPos);
            const auto newRect = node.getBoundingRect();
            update(oldRect.united(newRect));

            bool removedFromEveryTree = true;
            for (const auto tree : containingTrees) {
                if (tree->needsReinserting(node)) {
                    tree->remove(node);
                } else {
                    removedFromEveryTree = false;
                    tree->update(node);
                }
            }

            if (removedFromEveryTree) {
                m_quadTree.insert(node);
            }
        }
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void GraphManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setFlag(ItemIsSelectable, false);
        if (m_draggingNode) {
            m_draggingNode = false;
        } else if (m_pressedEmptySpace && !(event->modifiers() & Qt::ControlModifier)) {
            if (m_editingEnabled && addNode(event->pos().toPoint())) {
                m_graphStorage->recomputeAfterAddingNode(m_nodes.size());
            }
            m_pressedEmptySpace = false;
        }
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void GraphManager::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_editingEnabled) {
        removeSelectedNodes();
    } else if (event->key() == Qt::Key_F5) {
        if (event->modifiers() & Qt::ShiftModifier) {
            buildFullEdgeCache();
        } else {
            buildVisibleEdgeCache();
        }
    }

    QGraphicsObject::keyReleaseEvent(event);
}

void GraphManager::drawEdgeCache(QPainter* painter, qreal lod) const {
    if (!m_drawEdges) {
        return;
    }

    painter->setPen(QPen{QColor::fromRgb(m_nodeOutlineDefaultColor), 2.f});
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_edgesCache);
}

void GraphManager::drawNodes(QPainter* painter, qreal lod) const {
    if (lod <= 0.15) {
        return;
    }

    const auto outlineColor = QColor::fromRgb(m_nodeOutlineDefaultColor);
    if (!m_drawNodes) {
        for (const auto nodeIndex : m_selectedNodes) {
            const auto& node = m_nodes[nodeIndex];
            const auto& rect = node.getBoundingRect();

            painter->setPen(QPen{node.isSelected() ? Qt::green : outlineColor, 1.5});
            painter->setBrush(node.getFillColor());
            painter->drawEllipse(rect);
            if (lod >= 1) {
                painter->drawText(rect, Qt::AlignCenter, node.getLabel());
            }
        }

        return;
    }

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    for (const auto nodeIndex : visibleNodes) {
        const auto& node = m_nodes[nodeIndex];
        const auto& rect = node.getBoundingRect();

        if (lod >= 1) {
            m_graphStorage->forEachOutgoingEdge(
                nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                    const bool hasOppositeEdge = m_graphStorage->hasEdge(neighbourIndex, nodeIndex);
                    const auto oppositeCost =
                        hasOppositeEdge ? m_graphStorage->getCost(neighbourIndex, nodeIndex) : 0;

                    if (cost == 0 && oppositeCost == 0) {
                        return;
                    }

                    const auto& neighbour = m_nodes[neighbourIndex];
                    const auto mid = (node.getPosition() + neighbour.getPosition()) * 0.5f;
                    const auto direction = neighbour.getPosition() - node.getPosition();

                    QPointF normal(-direction.y(), direction.x());
                    const auto length = std::hypot(normal.x(), normal.y());
                    if (length > 0.0) {
                        normal /= length;
                    }

                    if (normal.y() > 0) {
                        normal = -normal;
                    }

                    const QFontMetrics fm(painter->font());
                    constexpr auto costOffset = 12.;

                    if (cost != 0) {
                        const auto costPos = mid + normal * costOffset;
                        const auto costText = QString::number(cost);
                        const auto textWidth = fm.horizontalAdvance(costText);
                        const auto textHeight = fm.height();

                        QRectF textRect(costPos.x() - textWidth / 2.0,
                                        costPos.y() - textHeight / 2.0, textWidth, textHeight);
                        painter->drawText(textRect, Qt::AlignCenter, costText);
                    }

                    if (hasOppositeEdge && oppositeCost != 0) {
                        const auto costPos = mid - normal * costOffset;
                        const auto costText = QString::number(oppositeCost);
                        const auto textWidth = fm.horizontalAdvance(costText);
                        const auto textHeight = fm.height();

                        QRectF textRect(costPos.x() - textWidth / 2.0,
                                        costPos.y() - textHeight / 2.0, textWidth, textHeight);
                        painter->drawText(textRect, Qt::AlignCenter, costText);
                    }
                });
        }

        painter->setPen(QPen{node.isSelected() ? Qt::green : outlineColor, 1.5});
        painter->setBrush(node.getFillColor());
        painter->drawEllipse(rect);
        if (lod >= 1) {
            painter->drawText(rect, Qt::AlignCenter, node.getLabel());
        }
    }
}

void GraphManager::drawQuadTree(QPainter* painter, QuadTree* quadTree, qreal lod) const {
    if (!m_drawQuadTrees || lod < 1.5) {
        return;
    }

    if (!quadTree || !isVisibleInScene(quadTree->getBoundary())) {
        return;
    }

    painter->setPen(Qt::gray);
    painter->setBrush(Qt::NoBrush);

    painter->drawRect(quadTree->getBoundary());
    if (!quadTree->isSubdivided()) {
        return;
    }

    const auto northWest = quadTree->getNorthWest();
    const auto northEast = quadTree->getNorthEast();
    const auto southWest = quadTree->getSouthWest();
    const auto southEast = quadTree->getSouthEast();

    drawQuadTree(painter, northWest, lod);
    drawQuadTree(painter, northEast, lod);
    drawQuadTree(painter, southWest, lod);
    drawQuadTree(painter, southEast, lod);
}

void GraphManager::addArrowToPath(QPainterPath& path, QPoint tip, const QPointF& dir) const {
    static constexpr double arrowLength = 10.0;
    static constexpr double arrowWidth = 5.0;

    QPointF normal(-dir.y(), dir.x());

    const auto base = tip - dir * arrowLength;
    const auto p1 = base + normal * arrowWidth;
    const auto p2 = base - normal * arrowWidth;

    path.moveTo(tip);
    path.lineTo(p1);
    path.lineTo(p2);
    path.closeSubpath();
}

void GraphManager::addEdgeToPath(QPainterPath& edgePath, NodeIndex_t nodeIndex,
                                 NodeIndex_t neighbourIndex, CostType_t cost) const {
    const bool hasOppositeEdge = m_graphStorage->hasEdge(neighbourIndex, nodeIndex);

    const auto srcCenter = m_nodes[nodeIndex].getPosition();
    const auto targetCenter = m_nodes[neighbourIndex].getPosition();

    const auto direction = targetCenter - srcCenter;
    const auto distance = std::hypot(direction.x(), direction.y());
    if (distance < 0.001) {
        return;
    }

    const auto directionNormalized = QPointF{direction.x() / distance, direction.y() / distance};

    const auto offset = (directionNormalized * NodeData::k_radius).toPoint();
    const auto lineStart = srcCenter + offset;
    const auto lineEnd = targetCenter - offset;

    if (m_orientedGraph && m_shouldDrawArrows) {
        addArrowToPath(edgePath, lineEnd, directionNormalized);
        if (hasOppositeEdge) {
            addArrowToPath(edgePath, lineStart, -directionNormalized);
        }
    }

    edgePath.moveTo(srcCenter);
    edgePath.lineTo(targetCenter);
}

bool GraphManager::isVisibleInScene(const QRect& rect) const {
    return m_sceneRect.intersects(rect);
}

void GraphManager::recomputeQuadTree() {
    m_quadTree.clear();
    for (const auto& node : m_nodes) {
        m_quadTree.insert(node);
    }
}

void GraphManager::switchToOptimalAdjacencyContainerIfNeeded() {
    const auto currentUsage = m_graphStorage->getMemoryUsage();
    if (m_graphStorage->type() == IGraphStorage::Type::ADJACENCY_LIST) {
        const auto matrixUsage =
            sizeof(AdjacencyMatrix) + m_nodes.size() * m_nodes.size() * sizeof(CostType_t);

        if (currentUsage > matrixUsage) {
            QMessageBox::information(
                nullptr, "Memory Usage",
                QString("Program has detected that by using an\nadjacency matrix "
                        "memory usage will be better.\n\nCurrent usage with list: "
                        "%1 bytes.\nPredicted usage with matrix: %2 bytes.")
                    .arg(currentUsage)
                    .arg(matrixUsage),
                QMessageBox::Ok);

            auto newStorage = std::make_unique<AdjacencyMatrix>();
            newStorage->resize(m_nodes.size());

            for (const auto& nodeData : m_nodes) {
                const NodeIndex_t i = nodeData.getIndex();
                if (m_graphStorage->hasEdge(i, i)) {
                    newStorage->addEdge(i, i, m_graphStorage->getCost(i, i));
                }

                m_graphStorage->forEachOutgoingEdgeWithOpposites(
                    i, [&](NodeIndex_t j, CostType_t cost) { newStorage->addEdge(i, j, cost); });
            }

            m_graphStorage = std::move(newStorage);
        }
    }
}

void GraphManager::removeSelectedNodes() {
    if (m_selectedNodes.empty()) {
        return;
    }

    m_graphStorage->recomputeBeforeRemovingNodes(m_nodes.size(), m_selectedNodes);

    for (NodeIndex_t index : m_selectedNodes) {
        if (index >= m_nodes.size()) {
            continue;
        }

        update(m_nodes[index].getBoundingRect());
        m_nodes.erase(m_nodes.begin() + index);
    }

    for (NodeIndex_t i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i].setIndex(i);
        update(m_nodes[i].getBoundingRect());
    }

    recomputeQuadTree();

    m_selectedNodes.clear();
}

void GraphManager::deselectNodes() {
    for (const auto nodeIndex : m_selectedNodes) {
        auto& node = m_nodes[nodeIndex];

        node.setSelected(false, 0);
        update(node.getBoundingRect());
    }

    m_selectedNodes.clear();
}

bool GraphManager::rasterizeEdgeAndCheckDensity(QPoint source, QPoint dest) {
    const auto sourceScreen = mapToScreen(source);
    const auto destScreen = mapToScreen(dest);

    QPoint gridStart(
        qBound(0, static_cast<int>(sourceScreen.x() * EDGE_GRID_SIZE), EDGE_GRID_SIZE - 1),
        qBound(0, static_cast<int>(sourceScreen.y() * EDGE_GRID_SIZE), EDGE_GRID_SIZE - 1));

    QPoint gridEnd(
        qBound(0, static_cast<int>(destScreen.x() * EDGE_GRID_SIZE), EDGE_GRID_SIZE - 1),
        qBound(0, static_cast<int>(destScreen.y() * EDGE_GRID_SIZE), EDGE_GRID_SIZE - 1));

    int dx = gridEnd.x() - gridStart.x();
    int dy = gridEnd.y() - gridStart.y();

    const int sx = (dx >= 0) ? 1 : -1;
    const int sy = (dy >= 0) ? 1 : -1;

    dx *= sx;
    dy *= sy;

    int err = dx - dy;

    int x = gridStart.x();
    int y = gridStart.y();

    while (true) {
        if (m_edgeDensity[y * EDGE_GRID_SIZE + x] >= MAX_EDGE_DENSITY) {
            return false;
        }

        ++m_edgeDensity[y * EDGE_GRID_SIZE + x];

        if (x == gridEnd.x() && y == gridEnd.y()) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    return true;
}

QPointF GraphManager::mapToScreen(QPointF graphPos) {
    const auto nx = (graphPos.x() - m_sceneRect.left()) / m_sceneRect.width();
    const auto ny = (graphPos.y() - m_sceneRect.top()) / m_sceneRect.height();
    return QPointF{nx, ny};
}

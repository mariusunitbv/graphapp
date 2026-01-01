#include <pch.h>

#include "GraphManager.h"

#include "storage/AdjacencyList.h"
#include "storage/AdjacencyMatrix.h"

#include "algorithms/IAlgorithm.h"

#include "../random/Random.h"

GraphManager::GraphManager() : m_graphStorage(std::make_unique<AdjacencyList>()) {
    setFlag(ItemIsFocusable);

    connect(&m_edgeWatcher, &QFutureWatcher<QPainterPath>::finished, [this]() {
        if (!m_edgeFuture.isCanceled()) {
            m_edgeCache = m_edgeWatcher.result();
            update();
        }
    });
}

void GraphManager::setGraphStorageType(IGraphStorage::Type type) {
    if (type == IGraphStorage::Type::ADJACENCY_LIST) {
        m_graphStorage = std::make_unique<AdjacencyList>();
    } else if (type == IGraphStorage::Type::ADJACENCY_MATRIX) {
        m_graphStorage = std::make_unique<AdjacencyMatrix>();
    } else {
        throw std::runtime_error("Unknown graph storage type.");
    }
}

const std::unique_ptr<IGraphStorage>& GraphManager::getGraphStorage() const {
    return m_graphStorage;
}

void GraphManager::setSceneDimensions(QSize size) {
    m_boundingRect.setCoords(0, 0, size.width(), size.height());
    m_quadTree.setBoundary(QRect(0, 0, size.width(), size.height()));
}

bool GraphManager::isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore) const {
    constexpr auto r = NodeData::k_radius;
    if (!m_boundingRect.adjusted(r, r, -r, -r).contains(pos)) {
        return false;
    }

    if (!m_collisionsCheckEnabled) {
        return true;
    }

    return !m_quadTree.intersectsAnotherNode(pos, 2 * NodeData::k_radius, nodeToIgnore);
}

void GraphManager::setCollisionsCheckEnabled(bool enabled) { m_collisionsCheckEnabled = enabled; }

void GraphManager::reset() {
    m_nodes.clear();
    m_quadTree.clear();
    m_edgeCache.clear();
    m_selectedNodes.clear();

    resetAdjacencyMatrix();
}

size_t GraphManager::getNodesCount() const { return m_nodes.size(); }

void GraphManager::reserveNodes(size_t count) { m_nodes.reserve(count); }

NodeData& GraphManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> GraphManager::getNode(const QPoint& pos, float minDistance) {
    return m_quadTree.getNodeAtPosition(pos, minDistance);
}

std::optional<NodeIndex_t> GraphManager::getSelectedNode() const {
    if (m_selectedNodes.empty()) {
        return std::optional<NodeIndex_t>();
    }

    return *m_selectedNodes.begin();
}

bool GraphManager::hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const {
    return m_graphStorage->getEdge(index, neighbour).has_value();
}

bool GraphManager::addNode(const QPoint& pos) {
    if (m_nodes.size() > NODE_LIMIT) {
        throw std::runtime_error("Cannot add more nodes: limit reached.");
    }

    if (!isGoodPosition(pos)) {
        return false;
    }

    auto& lastNode = m_nodes.emplace_back(m_nodes.size(), pos);
    lastNode.setFillColor(m_nodeDefaultColor);

    m_quadTree.insert(lastNode);

    update(lastNode.getBoundingRect());
    return true;
}

void GraphManager::addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost) {
    constexpr auto numBits = sizeof(CostType_t) * 8 - 1;
    constexpr auto maxCost = (1 << (numBits - 1)) - 1;
    constexpr auto minCost = -(1 << (numBits - 1));

    cost = std::clamp(cost, minCost, maxCost);
    m_graphStorage->addEdge(start, end, cost);
}

void GraphManager::randomlyAddEdges(size_t edgeCount) {
    std::unordered_set<uint64_t> used;
    used.reserve(edgeCount * 1.3);

    auto encode = [](size_t u, size_t v) -> uint64_t { return (uint64_t(u) << 32) | v; };

    while (used.size() < edgeCount) {
        NodeIndex_t u = static_cast<NodeIndex_t>(Random::get().getSize(0, m_nodes.size() - 1));
        if (u == m_nodes.size() - 1 && !m_orientedGraph) {
            continue;
        }

        NodeIndex_t v = static_cast<NodeIndex_t>(
            Random::get().getSize(m_orientedGraph ? 0 : u + 1, m_nodes.size() - 1));
        if (!m_allowLoops && u == v) {
            continue;
        }

        uint64_t key = encode(u, v);
        if (used.insert(key).second) {
            addEdge(u, v, 0);
            if (!m_orientedGraph) {
                addEdge(v, u, 0);
            }
        }
    }

    buildEdgeCache();
}

size_t GraphManager::getMaxEdgesCount() const {
    const size_t n = m_nodes.size();
    if (m_orientedGraph) {
        return m_allowLoops ? n * n : n * (n - 1);
    }

    return n * (n - 1) / 2;
}

void GraphManager::resizeAdjacencyMatrix(size_t nodeCount) { m_graphStorage->resize(nodeCount); }

void GraphManager::resetAdjacencyMatrix() { m_graphStorage = std::make_unique<AdjacencyList>(); }

void GraphManager::markEdgesDirty() { m_edgesDirty = true; }

void GraphManager::buildEdgeCache() {
    if (!m_drawEdges) {
        return;
    }

    if (!m_edgesDirty && m_edgeCache.m_edgePath.boundingRect().contains(m_sceneRect)) {
        if (m_edgeCache.m_builtWithLod >= 0.5) {
            return;
        }

        if (m_edgeCache.m_builtWithLod >= m_currentLod) {
            return;
        }
    }

    if (m_edgeFuture.isRunning()) {
        m_edgeFuture.cancel();
    }

    const auto halfVisibleWidth = m_sceneRect.width() / 2;
    const auto halfVisibleHeight = m_sceneRect.height() / 2;
    const auto extendedRect = m_sceneRect.adjusted(-halfVisibleWidth, -halfVisibleHeight,
                                                   halfVisibleWidth, halfVisibleHeight);

    std::vector<bool> visitMask(m_nodes.size(), false);
    std::vector<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(extendedRect, visitMask, visibleNodes);
    m_edgesDirty = false;

    m_edgeFuture = QtConcurrent::mappedReduced<EdgeCache>(
        visibleNodes,
        [&](NodeIndex_t nodeIndex) {
            EdgeCache cache;
            cache.m_builtWithLod = m_currentLod;

            if (m_allowLoops && hasNeighbour(nodeIndex, nodeIndex)) {
                const auto rect = m_nodes[nodeIndex].getBoundingRect();
                cache.m_loopEdgePath.addEllipse(rect.adjusted(8, 8, -8, -8));
            }

            m_graphStorage->forEachOutgoingEdge(
                nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                    addEdgeToPath(cache.m_edgePath, nodeIndex, neighbourIndex, cost);
                });

            return cache;
        },
        [](EdgeCache& result, const EdgeCache& edgeCache) {
            result.m_edgePath.addPath(edgeCache.m_edgePath);
            result.m_loopEdgePath.addPath(edgeCache.m_loopEdgePath);
            result.m_builtWithLod = edgeCache.m_builtWithLod;
        });

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

    buildEdgeCache();
}

void GraphManager::fillGraph() {
    reset();
    reserveNodes(NODE_LIMIT);

    constexpr int padding = 5;
    constexpr int step = 2 * NodeData::k_radius + padding;
    int x = NodeData::k_radius, y = NodeData::k_radius;

    for (size_t i = 0; i < NODE_LIMIT; ++i) {
        if (x + NodeData::k_radius > m_boundingRect.width()) {
            x = NodeData::k_radius;
            y += step;
        }

        if (y + NodeData::k_radius > m_boundingRect.height()) {
            break;
        }

        addNode({x, y});
        x += step;
    }

    resizeAdjacencyMatrix(m_nodes.size());
}

void GraphManager::setAllowEditing(bool enabled) { m_editingEnabled = enabled; }

bool GraphManager::getAllowEditing() const { return m_editingEnabled; }

void GraphManager::setAllowLoops(bool allow) { m_allowLoops = allow; }

bool GraphManager::getAllowLoops() const { return m_allowLoops; }

void GraphManager::setOrientedGraph(bool oriented) { m_orientedGraph = oriented; }

bool GraphManager::getOrientedGraph() const { return m_orientedGraph; }

void GraphManager::setDrawNodesEnabled(bool enabled) {
    m_drawNodes = enabled;
    update(m_sceneRect);
}

bool GraphManager::getDrawNodesEnabled() const { return m_drawNodes; }

void GraphManager::setDrawEdgesEnabled(bool enabled) {
    m_drawEdges = enabled;
    update(m_sceneRect);
}

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

void GraphManager::setNodeOutlineDefaultColor(QRgb color) {
    m_nodeOutlineDefaultColor = color;
    if (m_algorithmInfoTextItem) {
        m_algorithmInfoTextItem->setDefaultTextColor(QColor::fromRgb(color));
    }
}

void GraphManager::evaluateStorageStrategy(size_t edgeCount) {
    auto switchStorageToType = [this](IGraphStorage::Type type) {
        std::unique_ptr<IGraphStorage> newStorage;
        switch (type) {
            case IGraphStorage::Type::ADJACENCY_LIST:
                newStorage = std::make_unique<AdjacencyList>();
                break;
            case IGraphStorage::Type::ADJACENCY_MATRIX:
                newStorage = std::make_unique<AdjacencyMatrix>();
                break;
        }

        newStorage->resize(m_nodes.size());

        for (const auto& nodeData : m_nodes) {
            const NodeIndex_t i = nodeData.getIndex();
            const auto loop = m_graphStorage->getEdge(i, i);
            if (loop) {
                newStorage->addEdge(i, i, loop.value());
            }

            m_graphStorage->forEachOutgoingEdgeWithOpposites(
                i, [&](NodeIndex_t j, CostType_t cost) { newStorage->addEdge(i, j, cost); });
        }

        m_graphStorage = std::move(newStorage);
    };

    const auto listUsage = sizeof(AdjacencyList) +
                           m_nodes.size() * sizeof(AdjacencyList::Neighbours_t) +
                           edgeCount * sizeof(AdjacencyList::Neighbour_t);
    const auto matrixUsage =
        sizeof(AdjacencyMatrix) + m_nodes.size() * m_nodes.size() * sizeof(CostType_t);

    const bool switchToMatrix =
        listUsage > matrixUsage && m_graphStorage->type() == IGraphStorage::Type::ADJACENCY_LIST;
    const bool switchToList =
        matrixUsage > listUsage && m_graphStorage->type() == IGraphStorage::Type::ADJACENCY_MATRIX;

    if (!switchToMatrix && !switchToList) {
        return;
    }

    QMessageBox::information(nullptr, "Memory Usage",
                             QString("Program has detected that by using an adjacency \n%1 memory "
                                     "usage will be better.\n\nPredicted "
                                     "usage with list: %2 bytes.\nPredicted usage with "
                                     "matrix: %3 bytes.\nEdge count: %4")
                                 .arg(switchToMatrix ? "matrix" : "list")
                                 .arg(listUsage)
                                 .arg(matrixUsage)
                                 .arg(edgeCount),
                             QMessageBox::Ok);

    if (switchToMatrix) {
        switchStorageToType(IGraphStorage::Type::ADJACENCY_MATRIX);
    } else if (switchToList) {
        switchStorageToType(IGraphStorage::Type::ADJACENCY_LIST);
    }
}

bool GraphManager::runningAlgorithm() const { return !m_runningAlgorithms.empty(); }

void GraphManager::registerAlgorithm(IAlgorithm* algorithm) {
    m_runningAlgorithms.push_back(algorithm);

    if (!m_algorithmInfoTextItem) {
        auto font = QApplication::font();
        font.setPointSize(m_algorithmInfoTextSize);

        m_algorithmInfoTextItem = new QGraphicsTextItem(this);
        m_algorithmInfoTextItem->setPlainText(
            "Algorithm stats will be shown here.\nPress P to increase the size.\nPress L to "
            "decrease "
            "the size.");
        m_algorithmInfoTextItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_algorithmInfoTextItem->setDefaultTextColor(QColor::fromRgb(m_nodeOutlineDefaultColor));
        m_algorithmInfoTextItem->setZValue(10);
        m_algorithmInfoTextItem->setFont(font);

        updateAlgorithmInfoTextPos();
    }
}

void GraphManager::unregisterAlgorithm(IAlgorithm* algorithm) {
    auto it = std::find(m_runningAlgorithms.begin(), m_runningAlgorithms.end(), algorithm);

    if (it != m_runningAlgorithms.end()) {
        m_runningAlgorithms.erase(it);

        if (!runningAlgorithm()) {
            m_timeSinceAlgorithmFinished = std::chrono::steady_clock::now();

            if (m_algorithmInfoTextItem) {
                scene()->removeItem(m_algorithmInfoTextItem);
                delete m_algorithmInfoTextItem;
                m_algorithmInfoTextItem = nullptr;
            }
        }
    }
}

void GraphManager::cancelAlgorithms() {
    for (auto algorithm : m_runningAlgorithms) {
        algorithm->cancelAlgorithm();
    }
}

void GraphManager::addAlgorithmEdge(NodeIndex_t start, NodeIndex_t end, size_t priority) {
    if (!m_addingAlgorithmEdgesAllowed) {
        return;
    }

    auto& path = m_algorithmPaths[priority].m_path;

    const auto srcCenter = m_nodes[start].getPosition();
    const auto targetCenter = m_nodes[end].getPosition();

    path.moveTo(srcCenter);
    path.lineTo(targetCenter);

    if (m_orientedGraph) {
        const auto direction = targetCenter - srcCenter;
        const auto distance = std::hypot(direction.x(), direction.y());
        if (distance < 0.001) {
            return;
        }

        const auto directionNormalized =
            QPointF{direction.x() / distance, direction.y() / distance};

        const auto offset = (directionNormalized * NodeData::k_radius).toPoint();
        const auto lineEnd = targetCenter - offset;

        addArrowToPath(m_algorithmPaths[priority].m_arrowPath, lineEnd, directionNormalized);
    }
}

void GraphManager::setAlgorithmPathColor(size_t priority, QRgb color) {
    m_algorithmPaths[priority].m_color = color;
}

void GraphManager::clearAlgorithmPath(size_t priority) {
    if (!m_algorithmPaths.contains(priority)) {
        return;
    }

    m_algorithmPaths[priority].m_path.clear();
    m_algorithmPaths[priority].m_arrowPath.clear();
}

void GraphManager::clearAlgorithmPaths() { m_algorithmPaths.clear(); }

void GraphManager::setAlgorithmInfoText(const QString& text) {
    if (m_algorithmInfoTextItem) {
        m_algorithmInfoTextItem->setPlainText(text);
    }
}

void GraphManager::disableAddingAlgorithmEdges() { m_addingAlgorithmEdgesAllowed = false; }

void GraphManager::enableAddingAlgorithmEdges() { m_addingAlgorithmEdgesAllowed = true; }

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
    if (m_nodes[startNode].getSelectOrder() > m_nodes[endNode].getSelectOrder()) {
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
            totalCost += static_cast<uint64_t>(m_graphStorage->getEdge(nextNode, at).value());
        }

        m_nodes[at].setFillColor(qRgb(255, 255, 0));
    }

    QMessageBox::information(nullptr, "Dijkstra Result",
                             QString("Path found with total cost: %1").arg(totalCost));
}

void GraphManager::updateVisibleSceneRect() {
    QGraphicsScene* scene = this->scene();
    if (!scene) {
        return;
    }

    QGraphicsView* view = scene->views().first();
    const QRect sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect().toRect();
    if (m_sceneRect != sceneRect) {
        m_sceneRect = sceneRect;

        updateAlgorithmInfoTextPos();
        update(m_sceneRect);
    }
}

QRectF GraphManager::boundingRect() const { return m_boundingRect; }

void GraphManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    m_shouldDrawArrows = lod >= 1;
    m_currentLod = lod;

    drawEdgeCache(painter);
    drawAlgorithmEdges(painter);
    drawNodes(painter);
    drawQuadTree(painter, &m_quadTree);
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
                m_nodes[nodeIndex].deselect();
                m_selectedNodes.erase(nodeIndex);
            } else {
                m_nodes[nodeIndex].select(static_cast<uint32_t>(m_selectedNodes.size()));
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
        !(event->modifiers() & Qt::ControlModifier) && !runningAlgorithm()) {
        const auto selectedIndex = *m_selectedNodes.begin();
        auto& node = getNode(selectedIndex);
        const auto desiredPos = event->pos().toPoint() + m_dragOffset;

        if (isGoodPosition(desiredPos, selectedIndex)) {
            m_draggingNode = true;
            setCursor(Qt::ClosedHandCursor);

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
            m_edgesDirty = true;
            m_draggingNode = false;
            setCursor(Qt::ArrowCursor);
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
    } else if (event->key() == Qt::Key_Escape && !runningAlgorithm()) {
        deselectNodes();
    } else if (event->key() == Qt::Key_P && runningAlgorithm() && m_algorithmInfoTextSize < 40) {
        m_algorithmInfoTextItem->setFont(
            QFont(QApplication::font().family(), ++m_algorithmInfoTextSize));
    } else if (event->key() == Qt::Key_L && runningAlgorithm() && m_algorithmInfoTextSize > 6) {
        m_algorithmInfoTextItem->setFont(
            QFont(QApplication::font().family(), --m_algorithmInfoTextSize));
    }

    QGraphicsObject::keyReleaseEvent(event);
}

void GraphManager::drawEdgeCache(QPainter* painter) const {
    if (!m_drawEdges) {
        return;
    }

    painter->setPen(QPen{
        QColor::fromRgb(runningAlgorithm() ? qRgb(200, 200, 200) : m_nodeOutlineDefaultColor), 2.});
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_edgeCache.m_edgePath);
}

void GraphManager::drawAlgorithmEdges(QPainter* painter) const {
    if (!runningAlgorithm() || !m_drawEdges) {
        return;
    }

    painter->setBrush(Qt::NoBrush);
    for (auto it = m_algorithmPaths.rbegin(); it != m_algorithmPaths.rend(); ++it) {
        painter->setPen(QPen{QColor::fromRgb(it->second.m_color), 3});
        painter->drawPath(it->second.m_path);

        if (m_drawNodes && m_orientedGraph && m_shouldDrawArrows) {
            painter->drawPath(it->second.m_arrowPath);
        }
    }
}

void GraphManager::drawNodes(QPainter* painter) const {
    if (m_currentLod <= 0.15) {
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
            if (m_currentLod >= 1) {
                painter->drawText(rect, Qt::AlignCenter, node.getLabel());
            }
        }

        return;
    }

    std::vector<bool> visitMask(m_nodes.size(), false);
    std::vector<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visitMask, visibleNodes);

    for (const auto nodeIndex : visibleNodes) {
        const auto& node = m_nodes[nodeIndex];
        const auto& rect = node.getBoundingRect();

        if (m_currentLod >= 1 && m_drawEdges) {
            m_graphStorage->forEachOutgoingEdge(
                nodeIndex, [&](NodeIndex_t neighbourIndex, CostType_t cost) {
                    const auto oppositeEdge =
                        m_orientedGraph ? m_graphStorage->getEdge(neighbourIndex, nodeIndex)
                                        : std::nullopt;
                    const auto oppositeCost = oppositeEdge.has_value() ? oppositeEdge.value() : 0;

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

                    if (oppositeEdge && oppositeCost != 0) {
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
        if (m_currentLod >= 1) {
            painter->drawText(rect, Qt::AlignCenter, node.getLabel());

            if (m_drawEdges) {
                const auto loop = m_graphStorage->getEdge(nodeIndex, nodeIndex);
                if (loop) {
                    const auto cost = loop.value();
                    if (cost != 0) {
                        painter->drawText(rect.translated(0, -12), Qt::AlignCenter,
                                          QString::number(cost));
                    }
                }
            }
        }
    }

    if (m_drawEdges) {
        painter->setPen(outlineColor);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_edgeCache.m_loopEdgePath);
    }
}

void GraphManager::drawQuadTree(QPainter* painter, QuadTree* quadTree) const {
    if (!m_drawQuadTrees || m_currentLod < 1.5) {
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

    const auto northWest = quadTree->getNorthWest().get();
    const auto northEast = quadTree->getNorthEast().get();
    const auto southWest = quadTree->getSouthWest().get();
    const auto southEast = quadTree->getSouthEast().get();

    drawQuadTree(painter, northWest);
    drawQuadTree(painter, northEast);
    drawQuadTree(painter, southWest);
    drawQuadTree(painter, southEast);
}

void GraphManager::updateAlgorithmInfoTextPos() {
    if (!m_algorithmInfoTextItem) {
        return;
    }

    QGraphicsView* view = scene()->views().first();
    if (view) {
        const auto topLeft = view->mapToScene(16, 16);
        const auto viewportWidth = view->viewport()->width() - 32;

        m_algorithmInfoTextItem->setPos(topLeft);
        m_algorithmInfoTextItem->setTextWidth(viewportWidth);
    }
}

void GraphManager::addArrowToPath(QPainterPath& path, QPoint tip, const QPointF& dir) const {
    static constexpr double arrowLength = 10.0;
    static constexpr double arrowWidth = 5.0;

    QPointF normal(-dir.y(), dir.x());

    const auto base = tip - dir * arrowLength;
    const auto p1 = base + normal * arrowWidth;
    const auto p2 = base - normal * arrowWidth;

    path.moveTo(p1);
    path.lineTo(tip);
    path.lineTo(p2);
}

void GraphManager::addEdgeToPath(QPainterPath& edgePath, NodeIndex_t nodeIndex,
                                 NodeIndex_t neighbourIndex, CostType_t cost) const {
    const auto srcCenter = m_nodes[nodeIndex].getPosition();
    const auto targetCenter = m_nodes[neighbourIndex].getPosition();

    const auto shouldSkipNearEdges = m_currentLod < 0.5;
    if (shouldSkipNearEdges) {
        const auto srcScreen = mapToScreen(srcCenter);
        const auto targetScreen = mapToScreen(targetCenter);

        const auto minDistance = [this]() {
            if (m_currentLod <= 0.1) {
                return 8.;
            } else if (m_currentLod <= 0.2) {
                return 7.;
            } else if (m_currentLod <= 0.3) {
                return 6.;
            }

            return 5.5;
        }();

        const auto dx = targetScreen.x() - srcScreen.x();
        const auto dy = targetScreen.y() - srcScreen.y();
        const auto distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < minDistance * minDistance) {
            return;
        }
    }

    const auto direction = targetCenter - srcCenter;
    const auto distance = std::hypot(direction.x(), direction.y());
    if (distance < 0.001) {
        return;
    }

    const auto directionNormalized = QPointF{direction.x() / distance, direction.y() / distance};

    const auto offset = (directionNormalized * NodeData::k_radius).toPoint();
    const auto lineStart = srcCenter + offset;
    const auto lineEnd = targetCenter - offset;

    if (m_drawNodes && m_orientedGraph && m_shouldDrawArrows) {
        addArrowToPath(edgePath, lineEnd, directionNormalized);
        if (hasNeighbour(neighbourIndex, nodeIndex)) {
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
    using namespace std::chrono;

    const auto now = steady_clock::now();
    if (duration_cast<milliseconds>(now - m_timeSinceAlgorithmFinished).count() < 750) {
        return;
    }

    for (const auto nodeIndex : m_selectedNodes) {
        auto& node = m_nodes[nodeIndex];

        node.deselect();
        update(node.getBoundingRect());
    }

    m_selectedNodes.clear();
}

QPointF GraphManager::mapToScreen(QPointF graphPos) const {
    const auto x = graphPos.x() - m_sceneRect.left();
    const auto y = graphPos.y() - m_sceneRect.top();

    return QPointF{x, y};
}

#include <pch.h>

#include "GraphManager.h"

GraphManager::GraphManager() {
    setFlag(ItemIsFocusable);
    connect(&m_sceneUpdateTimer, &QTimer::timeout, [this]() {
        QGraphicsView* view = scene()->views().first();
        QRect sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect().toRect();
        if (m_sceneRect != sceneRect) {
            m_sceneRect = sceneRect;
            update(m_sceneRect);
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
    m_adjacencyMatrix.reset();
    m_edgesCache.clear();
    m_selectedNodes.clear();
}

NodeIndex_t GraphManager::getNodesCount() const { return static_cast<NodeIndex_t>(m_nodes.size()); }

void GraphManager::reserveNodes(size_t count) { m_nodes.reserve(count); }

NodeData& GraphManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> GraphManager::getNode(const QPoint& pos) {
    return m_quadTree.getNodeAtPosition(pos, NodeData::k_radius);
}

bool GraphManager::hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const {
    return m_adjacencyMatrix.hasEdge(index, neighbour);
}

bool GraphManager::addNode(const QPoint& pos) {
    if (m_nodes.size() >= NODE_LIMIT || !isGoodPosition(pos)) {
        return false;
    }

    NodeData nodeData(m_nodes.size(), pos);
    m_nodes.push_back(nodeData);
    m_quadTree.insert(nodeData);

    update(nodeData.getBoundingRect());
    return true;
}

void GraphManager::addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost) {
    cost = std::clamp(cost, -64, 63);
    m_adjacencyMatrix.setEdge(start, end, cost);
}

void GraphManager::randomlyAddEdges(size_t edgeCount) {}

size_t GraphManager::getMaxEdgesCount() const {
    const size_t n = m_nodes.size();
    if (m_orientedGraph) {
        return m_allowLoops ? n * n : n * (n - 1);
    }

    return n * (n - 1) / 2;
}

void GraphManager::resizeAdjacencyMatrix(size_t nodeCount) { m_adjacencyMatrix.resize(nodeCount); }

void GraphManager::updateVisibleEdgeCache() {
    if (m_adjacencyMatrix.empty()) {
        return;
    }

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    m_edgesCache.clear();
    for (const auto nodeIndex : visibleNodes) {
        for (NodeIndex_t neighbourIndex = 0; neighbourIndex < m_nodes.size(); ++neighbourIndex) {
            if (!visibleNodes.contains(neighbourIndex)) {
                continue;
            }

            if (m_adjacencyMatrix.hasEdge(nodeIndex, neighbourIndex)) {
                if (m_edgesCache.elementCount() > SHOWN_EDGE_LIMIT) {
                    update(m_sceneRect);
                    return;
                }

                constexpr auto arrowSize = 14.;

                const auto srcCenter = m_nodes[nodeIndex].getPosition();
                const auto targetCenter = m_nodes[neighbourIndex].getPosition();

                const auto direction = targetCenter - srcCenter;
                const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());

                if (distanceBetweenNodes < 0.0001) {
                    continue;
                }

                const auto radiusOffset = direction * (NodeData::k_radius / distanceBetweenNodes);

                const auto lineStart = srcCenter + radiusOffset;
                auto lineEnd = targetCenter - radiusOffset;
                const auto angle = std::atan2(-direction.y(), direction.x());
                const auto arrowP1 = lineEnd - QPoint(sin(angle + M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI / 3) * arrowSize);
                const auto arrowP2 = lineEnd - QPoint(sin(angle + M_PI - M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI - M_PI / 3) * arrowSize);
                if (m_shouldDrawArrows && m_orientedGraph) {
                    m_edgesCache.moveTo(lineEnd);
                    m_edgesCache.lineTo(arrowP1);
                    m_edgesCache.lineTo(arrowP2);
                    m_edgesCache.lineTo(lineEnd);
                    lineEnd = (arrowP1 + arrowP2) / 2;
                }

                if (m_orientedGraph) {
                    const auto directionVec = lineEnd - lineStart;
                    QPointF normalVec(-directionVec.y(), directionVec.x());
                    normalVec /= std::hypot(normalVec.x(), normalVec.y());

                    constexpr double curvature = 0.1;
                    const auto controlPoint =
                        (lineStart + lineEnd) / 2.0 + normalVec * curvature * distanceBetweenNodes;

                    m_edgesCache.moveTo(lineStart);
                    m_edgesCache.quadTo(controlPoint, lineEnd);
                } else {
                    m_edgesCache.moveTo(lineStart);
                    m_edgesCache.lineTo(lineEnd);
                }
            }
        }
    }

    update(m_sceneRect);
}

void GraphManager::updateFullEdgeCache() {
    if (m_adjacencyMatrix.empty()) {
        return;
    }

    m_edgesCache.clear();
    for (NodeIndex_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        for (NodeIndex_t neighbourIndex = 0; neighbourIndex < m_nodes.size(); ++neighbourIndex) {
            if (m_adjacencyMatrix.hasEdge(nodeIndex, neighbourIndex)) {
                constexpr auto arrowSize = 14.;

                const auto srcCenter = m_nodes[nodeIndex].getPosition();
                const auto targetCenter = m_nodes[neighbourIndex].getPosition();

                const auto direction = targetCenter - srcCenter;
                const auto distanceBetweenNodes = std::hypot(direction.x(), direction.y());

                if (distanceBetweenNodes < 0.0001) {
                    continue;
                }

                const auto radiusOffset = direction * (NodeData::k_radius / distanceBetweenNodes);

                const auto lineStart = srcCenter + radiusOffset;
                auto lineEnd = targetCenter - radiusOffset;
                const auto angle = std::atan2(-direction.y(), direction.x());
                const auto arrowP1 = lineEnd - QPoint(sin(angle + M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI / 3) * arrowSize);
                const auto arrowP2 = lineEnd - QPoint(sin(angle + M_PI - M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI - M_PI / 3) * arrowSize);
                if (m_orientedGraph) {
                    m_edgesCache.moveTo(lineEnd);
                    m_edgesCache.lineTo(arrowP1);
                    m_edgesCache.lineTo(arrowP2);
                    m_edgesCache.lineTo(lineEnd);
                    lineEnd = (arrowP1 + arrowP2) / 2;

                    const auto directionVec = lineEnd - lineStart;
                    QPointF normalVec(-directionVec.y(), directionVec.x());
                    normalVec /= std::hypot(normalVec.x(), normalVec.y());

                    constexpr double curvature = 0.1;
                    const auto controlPoint =
                        (lineStart + lineEnd) / 2.0 + normalVec * curvature * distanceBetweenNodes;

                    m_edgesCache.moveTo(lineStart);
                    m_edgesCache.quadTo(controlPoint, lineEnd);
                } else {
                    m_edgesCache.moveTo(lineStart);
                    m_edgesCache.lineTo(lineEnd);
                }
            }
        }
    }

    update();
}

void GraphManager::resetAdjacencyMatrix() { m_adjacencyMatrix.reset(); }

void GraphManager::completeGraph() {
    m_adjacencyMatrix.complete();
    updateVisibleEdgeCache();
}

void GraphManager::fillGraph() {}

void GraphManager::enableEditing() { m_editingEnabled = true; }

void GraphManager::disableEditing() { m_editingEnabled = false; }

void GraphManager::setAnimationsDisabled(bool disabled) { m_animationsDisabled = disabled; }

bool GraphManager::getAnimationsDisabled() const { return m_animationsDisabled; }

void GraphManager::setAllowLoops(bool allow) { m_allowLoops = allow; }

bool GraphManager::getAllowLoops() const { return m_allowLoops; }

void GraphManager::setOrientedGraph(bool oriented) { m_orientedGraph = oriented; }

bool GraphManager::getOrientedGraph() const { return m_orientedGraph; }

QRectF GraphManager::boundingRect() const { return m_boundingRect; }

void GraphManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    m_shouldDrawArrows = lod >= 1;
    painter->setPen(Qt::black);
    painter->drawPath(m_edgesCache);

    for (const auto nodeIndex : visibleNodes) {
        const auto& node = m_nodes[nodeIndex];
        const auto& rect = node.getBoundingRect();

        painter->setPen(QPen{node.m_selected ? Qt::green : Qt::black, 1.5});
        painter->setBrush(node.getFillColor());
        painter->drawEllipse(rect);
        if (lod >= 1) {
            painter->drawText(rect, Qt::AlignCenter, node.m_label);
        }
    }

    painter->setBrush(Qt::NoBrush);
    if (lod >= 1.5) {
        painter->setPen(Qt::gray);
        drawQuadTree(painter, &m_quadTree);
    }
}

void GraphManager::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::AltModifier)) {
        setFlag(ItemIsSelectable);

        const auto point = event->pos().toPoint();
        const auto nodeOpt = getNode(point);
        if (nodeOpt.has_value()) {
            if (!(event->modifiers() & Qt::ControlModifier)) {
                deselectNodes();
            }

            const auto nodeIndex = nodeOpt.value();
            m_nodes[nodeIndex].m_selected = true;
            m_selectedNodes.emplace(nodeIndex);
            m_dragOffset = getNode(nodeIndex).getPosition() - point;
        } else {
            if (!m_selectedNodes.empty()) {
                deselectNodes();
            } else {
                m_pressedEmptySpace = true;
            }
        }
    }

    QGraphicsObject::mousePressEvent(event);
}

void GraphManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && m_selectedNodes.size() == 1) {
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
        } else if (m_pressedEmptySpace) {
            if (addNode(event->pos().toPoint())) {
                recomputeAdjacencyMatrix();
            }
            m_pressedEmptySpace = false;
        }
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void GraphManager::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelectedNodes();
    } else if (event->key() == Qt::Key_F5) {
        if (event->modifiers() & Qt::ShiftModifier) {
            updateFullEdgeCache();
        } else {
            updateVisibleEdgeCache();
        }
    }

    QGraphicsObject::keyReleaseEvent(event);
}

void GraphManager::drawQuadTree(QPainter* painter, QuadTree* quadTree) const {
    if (!quadTree || !isVisibleInScene(quadTree->getBoundary())) {
        return;
    }

    painter->drawRect(quadTree->getBoundary());
    if (!quadTree->isSubdivided()) {
        return;
    }

    const auto northWest = quadTree->getNorthWest();
    const auto northEast = quadTree->getNorthEast();
    const auto southWest = quadTree->getSouthWest();
    const auto southEast = quadTree->getSouthEast();

    drawQuadTree(painter, northWest);
    drawQuadTree(painter, northEast);
    drawQuadTree(painter, southWest);
    drawQuadTree(painter, southEast);
}

bool GraphManager::isVisibleInScene(const QRect& rect) const {
    return m_sceneRect.intersects(rect);
}

void GraphManager::removeSelectedNodes() {
    if (m_selectedNodes.empty()) {
        return;
    }

    removeEdgesContainingSelectedNodes();

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
    updateVisibleEdgeCache();

    m_selectedNodes.clear();
}

void GraphManager::recomputeQuadTree() {
    m_quadTree.clear();
    for (const auto& node : m_nodes) {
        m_quadTree.insert(node);
    }
}

void GraphManager::recomputeAdjacencyMatrix() {
    AdjacencyMatrix newMatrix;
    newMatrix.resize(m_nodes.size());

    for (NodeIndex_t i = 0; i < m_nodes.size() - 1; ++i) {
        for (NodeIndex_t j = 0; j < m_nodes.size() - 1; ++j) {
            if (m_adjacencyMatrix.hasEdge(i, j)) {
                newMatrix.setEdge(i, j, m_adjacencyMatrix.getCost(i, j));
            }
        }
    }

    m_adjacencyMatrix = std::move(newMatrix);
}

void GraphManager::removeEdgesContainingSelectedNodes() {
    AdjacencyMatrix newMatrix;
    newMatrix.resize(m_nodes.size() - m_selectedNodes.size());

    for (NodeIndex_t i = 0, new_i = 0; i < m_nodes.size(); ++i) {
        if (m_selectedNodes.contains(i)) {
            continue;
        }

        for (NodeIndex_t j = 0, new_j = 0; j < m_nodes.size(); ++j) {
            if (m_selectedNodes.contains(j)) {
                continue;
            }

            if (m_adjacencyMatrix.hasEdge(i, j)) {
                newMatrix.setEdge(new_i, new_j, m_adjacencyMatrix.getCost(i, j));
            }

            ++new_j;
        }

        ++new_i;
    }

    m_adjacencyMatrix = std::move(newMatrix);
}

void GraphManager::deselectNodes() {
    for (const auto nodeIndex : m_selectedNodes) {
        auto& node = m_nodes[nodeIndex];

        node.m_selected = false;
        update(node.getBoundingRect());
    }

    m_selectedNodes.clear();
}

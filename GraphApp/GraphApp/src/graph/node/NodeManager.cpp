#include <pch.h>

#include "NodeManager.h"

NodeManager::NodeManager() {
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

void NodeManager::setSceneDimensions(qreal width, qreal height) {
    m_boundingRect.setCoords(0, 0, width, height);
    m_quadTree.setBoundary(QRect(0, 0, width, height));

    m_sceneUpdateTimer.start(200);
}

bool NodeManager::isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore) const {
    constexpr auto r = Node::k_radius;
    if (!m_boundingRect.adjusted(r, r, -r, -r).contains(pos)) {
        return false;
    }

    if (!m_collisionsCheckEnabled) {
        return true;
    }

    return !m_quadTree.intersectsAnotherNode(pos, nodeToIgnore);
}

void NodeManager::setCollisionsCheckEnabled(bool enabled) { m_collisionsCheckEnabled = enabled; }

void NodeManager::reset() {
    m_nodes.clear();
    m_quadTree.clear();
    m_adjacencyMatrix.reset();
    m_edgesCache.clear();

    m_selectedNodeIndex = INVALID_NODE;
}

NodeIndex_t NodeManager::getNodesCount() const { return static_cast<NodeIndex_t>(m_nodes.size()); }

void NodeManager::reserveNodes(size_t count) { m_nodes.reserve(count); }

NodeData& NodeManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> NodeManager::getNode(const QPoint& pos) {
    return m_quadTree.getNodeAtPosition(pos, Node::k_radius);
}

bool NodeManager::hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const {
    return m_adjacencyMatrix.hasEdge(index, neighbour);
}

bool NodeManager::addNode(const QPoint& pos) {
    if (m_nodes.size() >= NODE_LIMIT || !isGoodPosition(pos)) {
        return false;
    }

    NodeData nodeData(m_nodes.size(), pos);
    m_nodes.push_back(nodeData);
    m_quadTree.insert(nodeData);

    update(nodeData.getBoundingRect());
    return true;
}

void NodeManager::addEdge(NodeIndex_t start, NodeIndex_t end, int cost) {
    m_adjacencyMatrix.setEdge(start, end, cost);
}

void NodeManager::removeEdgesContaining(NodeIndex_t index) {
    AdjacencyMatrix newMatrix;
    newMatrix.resize(m_nodes.size() - 1);

    for (NodeIndex_t i = 0, new_i = 0; i < m_nodes.size(); ++i) {
        if (i == index) {
            continue;
        }

        for (NodeIndex_t j = 0, new_j = 0; j < m_nodes.size(); ++j) {
            if (j == index) {
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

void NodeManager::resizeAdjacencyMatrix(size_t nodeCount) { m_adjacencyMatrix.resize(nodeCount); }

void NodeManager::updateEdgeCache() {
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
                const auto lineEnd = targetCenter - radiusOffset;
                const auto angle = std::atan2(-direction.y(), direction.x());
                const auto arrowP1 = lineEnd - QPoint(sin(angle + M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI / 3) * arrowSize);
                const auto arrowP2 = lineEnd - QPoint(sin(angle + M_PI - M_PI / 3) * arrowSize,
                                                      cos(angle + M_PI - M_PI / 3) * arrowSize);
                m_edgesCache.moveTo(lineEnd);
                m_edgesCache.lineTo(arrowP1);
                m_edgesCache.lineTo(arrowP2);
                m_edgesCache.lineTo(lineEnd);

                m_edgesCache.moveTo(lineStart);
                m_edgesCache.lineTo(lineEnd);
            }
        }
    }

    update(m_sceneRect);
}

void NodeManager::resetAdjacencyMatrix() { m_adjacencyMatrix.reset(); }

void NodeManager::completeGraph() {
    m_adjacencyMatrix.complete();
    updateEdgeCache();
}

QRectF NodeManager::boundingRect() const { return m_boundingRect; }

void NodeManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    painter->setPen(Qt::black);
    painter->drawPath(m_edgesCache);

    for (const auto nodeIndex : visibleNodes) {
        const auto& node = m_nodes[nodeIndex];
        const auto& rect = node.getBoundingRect();

        painter->setPen(QPen{node.getIndex() == m_selectedNodeIndex ? Qt::green : Qt::black, 1.5});
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

void NodeManager::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::AltModifier)) {
        setFlag(ItemIsSelectable);

        const auto point = event->pos().toPoint();
        const auto nodeOpt = getNode(point);
        if (nodeOpt.has_value()) {
            m_selectedNodeIndex = nodeOpt.value();
            m_dragOffset = getNode(m_selectedNodeIndex).getPosition() - point;
        } else {
            if (m_selectedNodeIndex != std::numeric_limits<NodeIndex_t>::max()) {
                update(getNode(m_selectedNodeIndex).getBoundingRect());
                m_selectedNodeIndex = std::numeric_limits<NodeIndex_t>::max();
            } else {
                m_pressedEmptySpace = true;
            }
        }
    }

    QGraphicsObject::mousePressEvent(event);
}

void NodeManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        if (m_selectedNodeIndex != std::numeric_limits<NodeIndex_t>::max()) {
            auto& node = getNode(m_selectedNodeIndex);
            const auto desiredPos = event->pos().toPoint() + m_dragOffset;

            if (isGoodPosition(desiredPos, m_selectedNodeIndex)) {
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
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void NodeManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setFlag(ItemIsSelectable, false);
        if (m_draggingNode) {
            updateEdgeCache();
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

void NodeManager::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelectedNode();
    } else if (event->key() == Qt::Key_F5) {
        updateEdgeCache();
    }

    QGraphicsObject::keyReleaseEvent(event);
}

void NodeManager::drawQuadTree(QPainter* painter, QuadTree* quadTree) const {
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

bool NodeManager::isVisibleInScene(const QRect& rect) const { return m_sceneRect.intersects(rect); }

void NodeManager::removeSelectedNode() {
    if (m_selectedNodeIndex == INVALID_NODE) {
        return;
    }

    NodeIndex_t index = m_selectedNodeIndex;
    for (auto it = m_nodes.begin() + m_selectedNodeIndex; it != m_nodes.end();) {
        auto& node = *it;
        if (node.getIndex() == m_selectedNodeIndex) {
            removeEdgesContaining(node.getIndex());
            it = m_nodes.erase(it);
            m_selectedNodeIndex = INVALID_NODE;
        } else {
            node.setIndex(index++);
            ++it;
        }

        update(node.getBoundingRect());
    }

    recomputeQuadTree();
    updateEdgeCache();
}

void NodeManager::recomputeQuadTree() {
    m_quadTree.clear();
    for (const auto& node : m_nodes) {
        m_quadTree.insert(node);
    }
}

void NodeManager::recomputeAdjacencyMatrix() {
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

AdjacencyMatrix::AdjacencyMatrix(AdjacencyMatrix&& rhs) noexcept
    : m_nodeCount(rhs.m_nodeCount), m_matrix(std::move(rhs.m_matrix)) {}

AdjacencyMatrix& AdjacencyMatrix::operator=(AdjacencyMatrix&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    m_nodeCount = rhs.m_nodeCount;
    m_matrix = std::move(rhs.m_matrix);

    return *this;
}

void AdjacencyMatrix::resize(size_t nodeCount) {
    m_nodeCount = static_cast<NodeIndex_t>(nodeCount);
    m_matrix.resize(nodeCount * nodeCount, 0);
}

bool AdjacencyMatrix::empty() const { return m_matrix.empty(); }

void AdjacencyMatrix::reset() { std::fill(m_matrix.begin(), m_matrix.end(), 0); }

void AdjacencyMatrix::complete() { std::fill(m_matrix.begin(), m_matrix.end(), 0x80); }

void AdjacencyMatrix::setEdge(NodeIndex_t i, NodeIndex_t j, uint8_t cost) {
    m_matrix[i * m_nodeCount + j] = encode(cost);
}

void AdjacencyMatrix::clearEdge(NodeIndex_t i, NodeIndex_t j) { m_matrix[i * m_nodeCount + j] = 0; }

bool AdjacencyMatrix::hasEdge(NodeIndex_t i, NodeIndex_t j) const { return read(i, j) >> 7; }

uint8_t AdjacencyMatrix::getCost(NodeIndex_t i, NodeIndex_t j) const { return read(i, j) & 0x7F; }

uint8_t AdjacencyMatrix::encode(uint8_t cost) { return (1u << 7) | (cost & 0x7F); }

uint8_t AdjacencyMatrix::read(NodeIndex_t i, NodeIndex_t j) const {
    return m_matrix[i * m_nodeCount + j];
}

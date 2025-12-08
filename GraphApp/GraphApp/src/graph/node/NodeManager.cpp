#include <pch.h>

#include "NodeManager.h"

NodeManager::NodeManager() {
    setFlag(ItemIsFocusable);
    connect(&m_sceneUpdateTimer, &QTimer::timeout, [this]() {
        QGraphicsView* view = scene()->views().first();
        QRect sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect().toRect();
        if (m_sceneRect != sceneRect) {
            m_sceneRect = sceneRect;
            updateEdgeCache();
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
    m_adjacencyList.clear();
}

NodeIndex_t NodeManager::getNodesCount() const { return static_cast<NodeIndex_t>(m_nodes.size()); }

NodeData& NodeManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> NodeManager::getNode(const QPoint& pos) {
    return m_quadTree.getNodeAtPosition(pos, Node::k_radius);
}

bool NodeManager::hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const {
    if (!m_adjacencyList.contains(index)) {
        return false;
    }

    return m_adjacencyList.at(index).contains(neighbour);
}

bool NodeManager::hasNeighbours(NodeIndex_t index) const { return m_adjacencyList.contains(index); }

const Neighbours_t& NodeManager::getNeighbours(NodeIndex_t index) const {
    return m_adjacencyList.at(index);
}

void NodeManager::addNode(const QPoint& pos) {
    if (m_nodes.size() > 10000000 || !isGoodPosition(pos)) {
        return;
    }

    NodeData nodeData(m_nodes.size(), pos);
    m_nodes.push_back(nodeData);
    m_quadTree.insert(nodeData);

    update(nodeData.getBoundingRect());
}

void NodeManager::addEdge(NodeIndex_t start, NodeIndex_t end, int cost) {
    m_adjacencyList[start].emplace(end, cost);
}

void NodeManager::removeEdgesContaining(NodeIndex_t index) {
    m_adjacencyList.erase(index);
    AdjacencyList_t newAdjList;

    for (const auto& [nodeIdx, neighbours] : m_adjacencyList) {
        NodeIndex_t newNodeIdx = nodeIdx > index ? nodeIdx - 1 : nodeIdx;

        Neighbours_t newNeighbours;
        for (const auto& [neighbour, cost] : neighbours) {
            if (neighbour != index) {
                NodeIndex_t newNeighbour = neighbour > index ? neighbour - 1 : neighbour;
                newNeighbours.emplace(newNeighbour, cost);
            }
        }

        newAdjList[newNodeIdx] = std::move(newNeighbours);
    }

    m_adjacencyList = std::move(newAdjList);
}

void NodeManager::updateEdgeCache() {
    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    m_edgesCache.clear();
    for (const auto nodeIndex : visibleNodes) {
        for (const auto& [neighbourIndex, cost] : m_adjacencyList[nodeIndex]) {
            if (m_edgesCache.elementCount() > 100000) {
                return;
            }

            m_edgesCache.moveTo(m_nodes[nodeIndex].getPosition());
            m_edgesCache.lineTo(m_nodes[neighbourIndex].getPosition());
        }
    }

    update(m_sceneRect);
}

void NodeManager::resetAdjacencyList() { m_adjacencyList.clear(); }

void NodeManager::completeUnorientedGraph() {
    for (NodeIndex_t i = 0; i < m_nodes.size(); ++i) {
        for (NodeIndex_t j = i + 1; j < m_nodes.size(); ++j) {
            addEdge(i, j, 0);
            addEdge(j, i, 0);
        }
    }

    updateEdgeCache();
}

void NodeManager::completeOrientedGraph(bool allowLoops) {
    m_adjacencyList.reserve(m_nodes.size());

    for (NodeIndex_t i = 0; i < m_nodes.size(); ++i) {
        auto& neighbors = m_adjacencyList[i];
        neighbors.reserve(m_nodes.size());

        for (NodeIndex_t j = 0; j < m_nodes.size(); ++j) {
            if (i == j && !allowLoops) {
                continue;
            }

            neighbors.emplace_hint(neighbors.end(), j, 0);
        }
    }

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
            addNode(event->pos().toPoint());
            m_pressedEmptySpace = false;
        }
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void NodeManager::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelectedNode();
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

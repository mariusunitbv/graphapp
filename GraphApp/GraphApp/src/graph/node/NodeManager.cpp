#include <pch.h>

#include "NodeManager.h"

NodeManager::NodeManager() {
    setFlag(ItemIsFocusable);
    connect(&m_sceneUpdateTimer, &QTimer::timeout, [this]() {
        QGraphicsView* view = scene()->views().first();
        QRectF sceneRect = view->mapToScene(view->viewport()->rect()).boundingRect();
        if (m_sceneRect != sceneRect) {
            m_sceneRect = sceneRect.toRect();
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
}

size_t NodeManager::getNodesCount() const { return m_nodes.size(); }

NodeData& NodeManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> NodeManager::getNode(const QPoint& pos) {
    return m_quadTree.getNodeAtPosition(pos, Node::k_radius);
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
    EdgeData edgeData(start, end, cost);
    m_edges.emplace_back(edgeData);
}

void NodeManager::drawQuadTree(QPainter* painter, QuadTree* quadTree) const {
    if (!quadTree || !isVisibleInScene(quadTree->getBoundary())) {
        return;
    }

    const auto northWest = quadTree->getNorthWest();
    const auto northEast = quadTree->getNorthEast();
    const auto southWest = quadTree->getSouthWest();
    const auto southEast = quadTree->getSouthEast();

    painter->drawRect(quadTree->getBoundary());

    if (northWest) {
        drawQuadTree(painter, northWest);
    }

    if (northEast) {
        drawQuadTree(painter, northEast);
    }

    if (southWest) {
        drawQuadTree(painter, southWest);
    }

    if (southEast) {
        drawQuadTree(painter, southEast);
    }
}

void NodeManager::reserveEdges(size_t edges) { m_edges.reserve(edges); }

QRectF NodeManager::boundingRect() const { return m_boundingRect; }

void NodeManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    painter->drawPixmap(mapToScene(0, 0), m_edgesCache);

    int drawnNodes = 0;
    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    for (const auto& nodeIndex : visibleNodes) {
        const auto& node = m_nodes[nodeIndex];

        const auto& rect = node.getBoundingRect();

        ++drawnNodes;
        painter->setPen(QPen{node.getIndex() == m_selectedNodeIndex ? Qt::green : Qt::black, 1.5});
        painter->setBrush(node.getFillColor());
        painter->drawEllipse(rect);
        if (lod >= 1) {
            painter->drawText(rect, Qt::AlignCenter, node.m_label);
        }
    }

    qDebug() << drawnNodes << " out of " << m_nodes.size() << '\n';

    painter->setBrush(Qt::NoBrush);
    if (lod >= 1.5) {
        painter->setPen(Qt::gray);
        drawQuadTree(painter, &m_quadTree);
    }
}

void NodeManager::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::AltModifier)) {
        const auto point = event->pos().toPoint();
        const auto nodeOpt = getNode(point);
        if (nodeOpt.has_value()) {
            setFlag(ItemIsSelectable);

            m_selectedNodeIndex = nodeOpt.value();
            m_dragOffset = getNode(m_selectedNodeIndex).getPosition() - point;
        } else {
            if (m_selectedNodeIndex != std::numeric_limits<NodeIndex_t>::max()) {
                update(getNode(m_selectedNodeIndex).getBoundingRect());
                m_selectedNodeIndex = std::numeric_limits<NodeIndex_t>::max();
            } else {
                addNode(point);
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
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void NodeManager::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelectedNodes();
    } else if (event->key() == Qt::Key_Space) {
        updateEdgeCache();
    }

    QGraphicsObject::keyReleaseEvent(event);
}

bool NodeManager::isVisibleInScene(const QRect& rect) const { return m_sceneRect.intersects(rect); }

void NodeManager::removeSelectedNodes() {
    if (m_selectedNodeIndex == std::numeric_limits<NodeIndex_t>::max()) {
        return;
    }

    NodeIndex_t index = m_selectedNodeIndex;
    for (auto it = m_nodes.begin() + m_selectedNodeIndex; it != m_nodes.end();) {
        auto& node = *it;
        if (node.getIndex() == m_selectedNodeIndex) {
            update(node.getBoundingRect());

            it = m_nodes.erase(it);
            m_selectedNodeIndex = std::numeric_limits<NodeIndex_t>::max();
        } else {
            update(node.getBoundingRect());

            node.setIndex(index++);
            ++it;
        }
    }

    recomputeQuadTree();
}

void NodeManager::recomputeQuadTree() {
    m_quadTree.clear();
    for (const auto& node : m_nodes) {
        m_quadTree.insert(node);
    }
}

void NodeManager::updateEdgeCache() {
    if (m_edges.empty()) {
        return;
    }

    m_edgesCache = QPixmap(m_boundingRect.size());
    m_edgesCache.fill(Qt::transparent);

    QPainter p(&m_edgesCache);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::black);

    const QPoint offset = m_sceneRect.topLeft();

    std::unordered_set<NodeIndex_t> visibleNodes;
    m_quadTree.getNodesInArea(m_sceneRect, visibleNodes);

    for (const auto& e : m_edges) {
        if (!visibleNodes.contains(e.m_startNode) && !visibleNodes.contains(e.m_endNode)) {
            continue;
        }

        p.drawLine(m_nodes[e.m_startNode].getPosition(), m_nodes[e.m_endNode].getPosition());
    }

    p.end();

    update();
}

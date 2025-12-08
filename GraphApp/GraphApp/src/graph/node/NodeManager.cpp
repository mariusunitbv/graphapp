#include <pch.h>

#include "NodeManager.h"

NodeManager::NodeManager() {
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

    return !m_quadTree.intersectsAnotherNode(pos, nodeToIgnore);
}

size_t NodeManager::getNodesCount() const { return m_nodes.size(); }

NodeData& NodeManager::getNode(NodeIndex_t index) { return m_nodes[index]; }

std::optional<NodeIndex_t> NodeManager::getNode(const QPoint& pos) {
    return m_quadTree.getNodeAtPosition(pos, Node::k_radius);
}

void NodeManager::addNode(const QPoint& pos) {
    if (m_nodes.size() > 10000000) {
        return;
    }

    if (!isGoodPosition(pos)) {
        return;
    }

    NodeData nodeData(m_nodes.size(), pos);
    m_nodes.push_back(nodeData);
    m_quadTree.insert(nodeData);

    update(nodeData.getBoundingRect());
}

void NodeManager::drawQuadTree(QPainter* painter, QuadTree* quadTree) {
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

QRectF NodeManager::boundingRect() const { return m_boundingRect; }

void NodeManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    int drawnNodes = 0;

    const auto lod = option->levelOfDetailFromTransform(painter->worldTransform());

    for (const auto& node : m_nodes) {
        const auto& rect = node.getBoundingRect();
        if (!isVisibleInScene(rect)) {
            continue;
        }

        ++drawnNodes;
        painter->setPen(QPen{Qt::black, 1.5});
        painter->drawEllipse(rect);
        if (lod >= 1) {
            painter->drawText(rect, Qt::AlignCenter, node.m_label);
        }
    }

    qDebug() << "drawn:" << drawnNodes << "out of " << m_nodes.size() << '\n';

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

            qDebug() << "Picked node index:" << nodeOpt.value() << '\n';
            m_selectedNodeIndex = nodeOpt.value();
            m_dragOffset = getNode(m_selectedNodeIndex).getPosition() - point;
        }
    }

    QGraphicsObject::mousePressEvent(event);
}

void NodeManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        if (m_selectedNodeIndex != std::numeric_limits<NodeIndex_t>::max()) {
            auto& node = getNode(m_selectedNodeIndex);
            const auto desiredPos = event->pos() + m_dragOffset;

            QuadTree* nodeTree = m_quadTree.getContainingQuadTree(node);
            if (!nodeTree->intersectsAnotherNode(desiredPos.toPoint(), m_selectedNodeIndex)) {
                const auto oldRect = node.getBoundingRect();
                node.setPosition(desiredPos);
                const auto newRect = node.getBoundingRect();
                update(oldRect.united(newRect));

                if (nodeTree->needsReinserting(node)) {
                    nodeTree->remove(node);
                    m_quadTree.insert(node);
                } else {
                    nodeTree->update(node);
                }
            } else {
                qDebug() << "Invalid move position for node index:" << m_selectedNodeIndex << '\n';
            }
        }
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void NodeManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_selectedNodeIndex != std::numeric_limits<NodeIndex_t>::max()) {
            setFlag(ItemIsSelectable, false);

            qDebug() << "Released node index:" << m_selectedNodeIndex << '\n';
            m_selectedNodeIndex = std::numeric_limits<NodeIndex_t>::max();
        }
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

bool NodeManager::isVisibleInScene(const QRect& rect) const { return m_sceneRect.intersects(rect); }

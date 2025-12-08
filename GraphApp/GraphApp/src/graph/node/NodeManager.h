#pragma once

#include "Node.h"
#include "QuadTree.h"

class NodeManager : public QGraphicsObject {
    Q_OBJECT

   public:
    NodeManager();

    void setSceneDimensions(qreal width, qreal height);
    bool isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore = -1) const;

    size_t getNodesCount() const;
    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos);

    void addNode(const QPoint& pos);

    void drawQuadTree(QPainter* painter, QuadTree* quadTree);

   protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

   private:
    bool isVisibleInScene(const QRect& rect) const;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;

    QRect m_boundingRect{};

    QTimer m_sceneUpdateTimer;
    QRect m_sceneRect{};

    NodeIndex_t m_selectedNodeIndex{std::numeric_limits<NodeIndex_t>::max()};
    QPoint m_dragOffset{};
};

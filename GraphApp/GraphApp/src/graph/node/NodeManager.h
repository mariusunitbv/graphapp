#pragma once

#include "Node.h"
#include "../edge/Edge.h"
#include "QuadTree.h"

class NodeManager : public QGraphicsObject {
    Q_OBJECT

   public:
    NodeManager();

    void setSceneDimensions(qreal width, qreal height);
    bool isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore = -1) const;
    void setCollisionsCheckEnabled(bool enabled);
    void reset();

    size_t getNodesCount() const;
    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos);

    void addNode(const QPoint& pos);
    void addEdge(NodeIndex_t start, NodeIndex_t end, int cost);

    void drawQuadTree(QPainter* painter, QuadTree* quadTree) const;

    void reserveEdges(size_t edges);

   protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

   private:
    bool isVisibleInScene(const QRect& rect) const;
    void removeSelectedNodes();
    void recomputeQuadTree();
    void updateEdgeCache();

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;

    std::vector<EdgeData> m_edges;
    QPixmap m_edgesCache;

    NodeIndex_t m_selectedNodeIndex{std::numeric_limits<NodeIndex_t>::max()};
    bool m_collisionsCheckEnabled{true};

    QPoint m_dragOffset{};
};

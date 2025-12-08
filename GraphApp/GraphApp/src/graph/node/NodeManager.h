#pragma once

#include "Node.h"
#include "QuadTree.h"

using Neighbours_t = std::unordered_map<NodeIndex_t, int>;
using AdjacencyList_t = std::unordered_map<NodeIndex_t, Neighbours_t>;

class NodeManager : public QGraphicsObject {
    Q_OBJECT

   public:
    NodeManager();

    void setSceneDimensions(qreal width, qreal height);
    bool isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore = -1) const;
    void setCollisionsCheckEnabled(bool enabled);
    void reset();

    NodeIndex_t getNodesCount() const;

    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos);

    bool hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const;
    bool hasNeighbours(NodeIndex_t index) const;
    const Neighbours_t& getNeighbours(NodeIndex_t index) const;

    void addNode(const QPoint& pos);

    void addEdge(NodeIndex_t start, NodeIndex_t end, int cost);
    void removeEdgesContaining(NodeIndex_t index);

    void updateEdgeCache();
    void resetAdjacencyList();

    void completeUnorientedGraph();
    void completeOrientedGraph(bool allowLoops);

   protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

   private:
    void drawQuadTree(QPainter* painter, QuadTree* quadTree) const;
    bool isVisibleInScene(const QRect& rect) const;
    void removeSelectedNode();
    void recomputeQuadTree();

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    QPainterPath m_edgesCache;

    AdjacencyList_t m_adjacencyList;

    NodeIndex_t m_selectedNodeIndex{INVALID_NODE};
    bool m_collisionsCheckEnabled{true};
    bool m_draggingNode{false};
    bool m_pressedEmptySpace{false};

    QPoint m_dragOffset{};
};

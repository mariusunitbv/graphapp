#pragma once

#include "Node.h"
#include "QuadTree.h"

constexpr size_t NODE_LIMIT = 100000;
constexpr size_t SHOWN_EDGE_LIMIT = 150000;

class AdjacencyMatrix {
   public:
    AdjacencyMatrix() = default;

    AdjacencyMatrix(const AdjacencyMatrix&) = delete;
    AdjacencyMatrix& operator=(const AdjacencyMatrix&) = delete;

    AdjacencyMatrix(AdjacencyMatrix&& rhs) noexcept;
    AdjacencyMatrix& operator=(AdjacencyMatrix&& rhs) noexcept;

    void resize(size_t nodeCount);
    bool empty() const;

    void reset();
    void complete();

    void setEdge(NodeIndex_t i, NodeIndex_t j, uint8_t cost);
    void clearEdge(NodeIndex_t i, NodeIndex_t j);

    bool hasEdge(NodeIndex_t i, NodeIndex_t j) const;
    uint8_t getCost(NodeIndex_t i, NodeIndex_t j) const;

   private:
    static uint8_t encode(uint8_t cost);
    uint8_t read(NodeIndex_t i, NodeIndex_t j) const;

    NodeIndex_t m_nodeCount{0};
    std::vector<uint8_t> m_matrix{};
};

class NodeManager : public QGraphicsObject {
    Q_OBJECT

   public:
    NodeManager();

    void setSceneDimensions(qreal width, qreal height);
    bool isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore = -1) const;
    void setCollisionsCheckEnabled(bool enabled);
    void reset();

    NodeIndex_t getNodesCount() const;
    void reserveNodes(size_t count);

    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos);

    bool hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const;

    bool addNode(const QPoint& pos);

    void addEdge(NodeIndex_t start, NodeIndex_t end, int cost);
    void removeEdgesContaining(NodeIndex_t index);

    void resizeAdjacencyMatrix(size_t nodeCount);
    void updateEdgeCache();
    void resetAdjacencyMatrix();
    void completeGraph();

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
    void recomputeAdjacencyMatrix();

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    QPainterPath m_edgesCache;
    AdjacencyMatrix m_adjacencyMatrix;

    NodeIndex_t m_selectedNodeIndex{INVALID_NODE};
    bool m_collisionsCheckEnabled{true};
    bool m_draggingNode{false};
    bool m_pressedEmptySpace{false};

    QPoint m_dragOffset{};
};

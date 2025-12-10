#pragma once

#include "AdjacencyMatrix.h"
#include "QuadTree.h"

constexpr size_t NODE_LIMIT = 100000;
constexpr size_t SHOWN_EDGE_LIMIT = 150000;

class GraphManager : public QGraphicsObject {
    Q_OBJECT

   public:
    GraphManager();

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
    void addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost);
    void randomlyAddEdges(size_t edgeCount);

    size_t getMaxEdgesCount() const;

    void resizeAdjacencyMatrix(size_t nodeCount);
    void updateVisibleEdgeCache();
    void updateFullEdgeCache();
    void resetAdjacencyMatrix();
    void completeGraph();
    void fillGraph();

    void enableEditing();
    void disableEditing();

    void setAnimationsDisabled(bool disabled);
    bool getAnimationsDisabled() const;

    void setAllowLoops(bool allow);
    bool getAllowLoops() const;

    void setOrientedGraph(bool oriented);
    bool getOrientedGraph() const;

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
    void removeSelectedNodes();
    void recomputeQuadTree();
    void recomputeAdjacencyMatrix();
    void removeEdgesContainingSelectedNodes();
    void deselectNodes();

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    QPainterPath m_edgesCache;
    AdjacencyMatrix m_adjacencyMatrix;

    std::set<NodeIndex_t, std::greater<NodeIndex_t>> m_selectedNodes{};

    bool m_collisionsCheckEnabled{true};
    bool m_draggingNode{false};
    bool m_pressedEmptySpace{false};
    bool m_animationsDisabled{false};
    bool m_editingEnabled{false};
    bool m_allowLoops{false};
    bool m_orientedGraph{true};
    bool m_shouldDrawArrows{true};

    QPoint m_dragOffset{};
};

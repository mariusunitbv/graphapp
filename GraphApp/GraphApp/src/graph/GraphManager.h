#pragma once

#include "AdjacencyMatrix.h"
#include "QuadTree.h"

constexpr size_t NODE_LIMIT = 100000;
constexpr size_t SHOWN_EDGE_LIMIT = 200000;
constexpr uint16_t EDGE_GRID_SIZE = 128;
constexpr uint16_t MAX_EDGE_DENSITY = 1024;

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
    void buildFullEdgeCache();
    void resetAdjacencyMatrix();
    void completeGraph();
    void fillGraph();

    void setAllowEditing(bool enabled);
    bool getAllowEditing() const;

    void setAnimationsDisabled(bool disabled);
    bool getAnimationsDisabled() const;

    void setAllowLoops(bool allow);
    bool getAllowLoops() const;

    void setOrientedGraph(bool oriented);
    bool getOrientedGraph() const;

    void setDrawNodesEnabled(bool enabled);
    bool getDrawNodesEnabled() const;

    void setDrawEdgesEnabled(bool enabled);
    bool getDrawEdgesEnabled() const;

    void setDrawQuadTreesEnabled(bool enabled);
    bool getDrawQuadTreesEnabled() const;

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
    bool rasterizeEdgeAndCheckDensity(QPoint source, QPoint dest);

    QPointF mapToScreen(QPointF graphPos);

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    QPainterPath m_edgesCache;
    AdjacencyMatrix m_adjacencyMatrix;

    std::array<uint16_t, EDGE_GRID_SIZE * EDGE_GRID_SIZE> m_edgeDensity{};
    std::set<NodeIndex_t, std::greater<NodeIndex_t>> m_selectedNodes{};

    QPoint m_dragOffset{};

    bool m_collisionsCheckEnabled : 1 {true};
    bool m_draggingNode : 1 {false};
    bool m_pressedEmptySpace : 1 {false};
    bool m_animationsDisabled : 1 {false};
    bool m_editingEnabled : 1 {false};
    bool m_allowLoops : 1 {false};
    bool m_orientedGraph : 1 {true};
    bool m_shouldDrawArrows : 1 {true};
    bool m_drawNodes : 1 {true};
    bool m_drawEdges : 1 {true};
    bool m_drawQuadTrees : 1 {false};
};

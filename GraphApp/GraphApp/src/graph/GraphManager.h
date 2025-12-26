#pragma once

#include "../form/loading_screen/LoadingScreen.h"

#include "storage/IGraphStorage.h"

#include "QuadTree.h"

constexpr size_t NODE_LIMIT = INVALID_NODE - 1;

class GraphManager : public QGraphicsObject {
    Q_OBJECT

   public:
    friend class Graph;

    GraphManager();

    void setGraphStorageType(IGraphStorage::Type type);
    const std::unique_ptr<IGraphStorage>& getGraphStorage() const;

    void setSceneDimensions(QSize size);
    bool isGoodPosition(const QPoint& pos, NodeIndex_t nodeToIgnore = -1) const;
    void setCollisionsCheckEnabled(bool enabled);
    void reset();

    size_t getNodesCount() const;
    void reserveNodes(size_t count);

    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos, float minDistance = NodeData::k_radius);

    bool hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const;

    bool addNode(const QPoint& pos);
    void addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost);
    void randomlyAddEdges(size_t edgeCount);

    size_t getMaxEdgesCount() const;

    void resizeAdjacencyMatrix(size_t nodeCount);
    void resetAdjacencyMatrix();

    void buildEdgeCache();

    void completeGraph();
    void fillGraph();

    void setAllowEditing(bool enabled);
    bool getAllowEditing() const;

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

    void setNodeDefaultColor(QRgb color);
    void setNodeOutlineDefaultColor(QRgb color);

    void evaluateStorageStrategy(size_t edgeCount);

    void dijkstra();

    QRectF boundingRect() const override;

   protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

   private:
    void drawEdgeCache(QPainter* painter, qreal lod) const;
    void drawNodes(QPainter* painter, qreal lod) const;
    void drawQuadTree(QPainter* painter, QuadTree* quadTree, qreal lod) const;

    void addArrowToPath(QPainterPath& path, QPoint tip, const QPointF& dir) const;
    void addEdgeToPath(QPainterPath& edgePath, NodeIndex_t nodeIndex, NodeIndex_t neighbourIndex,
                       CostType_t cost) const;

    bool isVisibleInScene(const QRect& rect) const;

    void recomputeQuadTree();

    void removeSelectedNodes();
    void deselectNodes();

    QPointF mapToScreen(QPointF graphPos) const;

    struct EdgeCache {
        void clear() {
            m_edgePath.clear();
            m_loopEdgePath.clear();
        }

        QPainterPath m_edgePath;
        QPainterPath m_loopEdgePath;
        qreal m_builtWithLod;
    };

    QRect m_boundingRect{};
    QRect m_sceneRect{};
    QTimer m_sceneUpdateTimer;

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    EdgeCache m_edgeCache;
    QPainterPath m_algorithmPath;
    std::unique_ptr<IGraphStorage> m_graphStorage{};

    std::set<NodeIndex_t, std::greater<NodeIndex_t>> m_selectedNodes{};

    QPoint m_dragOffset{};
    qreal m_currentLod{1.0};

    QFuture<EdgeCache> m_edgeFuture;
    QFutureWatcher<EdgeCache> m_edgeWatcher;

    bool m_collisionsCheckEnabled : 1 {true};
    bool m_draggingNode : 1 {false};
    bool m_pressedEmptySpace : 1 {false};
    bool m_editingEnabled : 1 {false};
    bool m_allowLoops : 1 {false};
    bool m_orientedGraph : 1 {true};
    bool m_shouldDrawArrows : 1 {true};
    bool m_drawNodes : 1 {true};
    bool m_drawEdges : 1 {true};
    bool m_drawQuadTrees : 1 {false};

    QRgb m_nodeDefaultColor{qRgb(255, 255, 255)};
    QRgb m_nodeOutlineDefaultColor{qRgb(255, 255, 255)};

    QPointer<LoadingScreen> m_loadingScreen{};
};

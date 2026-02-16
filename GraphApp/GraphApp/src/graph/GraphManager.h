#pragma once

#include "storage/IGraphStorage.h"

#include "QuadTree.h"

class IAlgorithm;

constexpr size_t NODE_LIMIT = 100'000'000;

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
    size_t getSelectedNodesCount() const;
    void reserveNodes(size_t count);

    NodeData& getNode(NodeIndex_t index);
    std::optional<NodeIndex_t> getNode(const QPoint& pos, float minDistance = NodeData::k_radius);
    std::optional<NodeIndex_t> getSelectedNode() const;
    std::optional<std::pair<NodeIndex_t, NodeIndex_t>> getTwoSelectedNodes() const;

    bool hasNeighbour(NodeIndex_t index, NodeIndex_t neighbour) const;

    bool addNode(const QPoint& pos);
    void addEdge(NodeIndex_t start, NodeIndex_t end, int32_t cost);
    void randomlyAddEdges(size_t edgeCount);

    size_t getMaxEdgesCount() const;

    void resizeAdjacencyMatrix(size_t nodeCount);
    void resetAdjacencyMatrix();

    void markEdgesDirty();
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

    bool runningAlgorithm() const;
    void registerAlgorithm(IAlgorithm* algorithm);
    void unregisterAlgorithm(IAlgorithm* algorithm);
    void cancelAlgorithms();

    void addAlgorithmEdge(NodeIndex_t start, NodeIndex_t end, size_t priority);
    void setAlgorithmPathColor(size_t priority, QRgb color);
    void clearAlgorithmPath(size_t priority);
    void clearAlgorithmPaths();
    void setAlgorithmInfoText(const QString& text);

    void disableAddingAlgorithmEdges();
    void enableAddingAlgorithmEdges();

    void increaseEdgeThickness();
    void decreaseEdgeThickness();

    void updateVisibleSceneRect();
    QRectF boundingRect() const override;

   protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

   private:
    void drawEdgeCache(QPainter* painter) const;
    void drawAlgorithmEdges(QPainter* painter) const;
    void drawEdgePreview(QPainter* painter) const;
    void drawNodes(QPainter* painter) const;
    void drawQuadTree(QPainter* painter, QuadTree* quadTree) const;
    void updateAlgorithmInfoTextPos();

    void addArrowToPath(QPainterPath& path, QPoint tip, const QPointF& dir) const;
    void addEdgeToPath(QPainterPath& edgePath, NodeIndex_t nodeIndex, NodeIndex_t neighbourIndex,
                       CostType_t cost) const;

    bool isVisibleInScene(const QRect& rect) const;

    void recomputeQuadTree();

    void removeSelectedNodes();
    void deselectNodes();

    void handleInteractiveEdgeAction(const QPoint& mousePos);

    QPointF mapToScreen(QPointF graphPos) const;

    struct EdgeCache {
        void clear() {
            m_edgePath.clear();
            m_loopEdgePath.clear();
            m_builtWithLod = 0;
        }

        QPainterPath m_edgePath;
        QPainterPath m_loopEdgePath;
        qreal m_builtWithLod{};
    };

    struct AlgorithmPath {
        QRgb m_color{qRgb(255, 0, 0)};
        QPainterPath m_path;
        QPainterPath m_arrowPath;
    };

    QRect m_boundingRect{};
    QRect m_sceneRect{};

    std::vector<NodeData> m_nodes;
    QuadTree m_quadTree;
    EdgeCache m_edgeCache;
    std::unique_ptr<IGraphStorage> m_graphStorage{};

    std::vector<IAlgorithm*> m_runningAlgorithms;
    std::map<int64_t, AlgorithmPath> m_algorithmPaths;
    QGraphicsTextItem* m_algorithmInfoTextItem{nullptr};
    uint8_t m_algorithmInfoTextSize{14};
    uint8_t m_additionalEdgeThickness{0};

    std::set<NodeIndex_t, std::greater<NodeIndex_t>> m_selectedNodes{};

    QPoint m_dragOffset{}, m_edgePreviewEndPoint{};
    qreal m_currentLod{1.0};

    NodeIndex_t m_edgePreviewStartNode{INVALID_NODE};

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
    bool m_edgesDirty : 1 {false};
    bool m_addingAlgorithmEdgesAllowed : 1 {true};

    QRgb m_nodeDefaultColor{qRgb(255, 255, 255)};
    QRgb m_nodeOutlineDefaultColor{qRgb(255, 255, 255)};
};

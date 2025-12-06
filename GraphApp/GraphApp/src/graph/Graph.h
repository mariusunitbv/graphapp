#pragma once
#pragma once

#include "node/Node.h"
#include "edge/Edge.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent = nullptr);
    ~Graph();

    void onAdjacencyListChanged(const QString& text);
    Node* getFirstSelectedNode() const;

    size_t getNodesCount() const;
    const std::vector<Node*>& getNodes() const;
    Node* getRandomNode() const;

    QPointF getGraphSize() const;

    const std::vector<Edge*>& getEdges() const;
    const std::unordered_map<Node*, std::unordered_set<Node*>>& getAdjacencyList() const;

    int getZoomPercentage();

    void resetGraph();

    void enableEditing();
    void disableEditing();

    void setAnimationsDisabled(bool disabled);
    bool getAnimationsDisabled() const;

    void setAllowLoops(bool allow);
    bool getAllowLoops() const;

    void setOrientedGraph(bool oriented);
    bool getOrientedGraph() const;

    Graph* getCopy() const;
    Graph* getInvertedGraph() const;

   signals:
    void zoomChanged();
    void movedGraph();
    void endedAlgorithm();
    void spacePressed();
    void escapePressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyPressEvent(QKeyEvent* event) final;

    void resizeEvent(QResizeEvent* event) final;

   public:
    void addNode(const QPointF& pos);

   private:
    void removeNodeConnections(Node* node);
    void removeSelectedNodes();
    void removeAllNodes();
    void onNodeMarkedForErasure(Node* node);

   public:
    void addEdge(Node* a, Node* b, int cost);
    size_t getMaxEdgesCount();
    void reserveEdges(size_t edges);

   private:
    void removeEdgesConnectedToNode(Node* node);
    void onEdgeMarkedForErasure(Edge* edge);

    void resetAdjacencyList();

    QGraphicsScene* m_scene;

    std::vector<Node*> m_nodes;
    std::vector<Edge*> m_edges;
    std::unordered_map<Node*, std::unordered_set<Node*>> m_adjacencyList;

    bool m_isDragging{false};
    bool m_isSelecting{false};
    bool m_animationsDisabled{false};
    bool m_editingEnabled{false};
    bool m_allowLoops{false};
    bool m_orientedGraph{true};

    double m_currentZoomScale{1.};

    static constexpr double k_minScale = 0.25;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.25;
};

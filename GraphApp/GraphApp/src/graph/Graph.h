#pragma once

#include "node/Node.h"
#include "edge/Edge.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent);

    void onAdjacencyListChanged(const QString& text);
    Node* getFirstSelectedNode() const;

    size_t getNodesCount();
    const std::vector<Node*>& getNodes() const;
    const std::unordered_map<Node*, std::unordered_set<Node*>>& getAdjacencyList() const;

    int getZoomPercentage();

    void resetGraph();

    void enableEditing();
    void disableEditing();

   signals:
    void zoomChanged();
    void movedGraph();
    void endedAlgorithm();
    void spacePressed();

   protected:
    void wheelEvent(QWheelEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyPressEvent(QKeyEvent* event) final;

    void resizeEvent(QResizeEvent* event) final;

   private:
    void addNode(const QPointF& pos);
    void removeNodeConnections(Node* node);
    void removeSelectedNodes();
    void onNodeMarkedForErasure(Node* node);

    void addEdge(Node* a, Node* b, int cost);
    void removeEdgesConnectedToNode(Node* node);
    void onEdgeMarkedForErasure(Edge* edge);

    void resetAdjacencyList();

    std::vector<Node*> m_nodes;
    std::vector<Edge*> m_edges;

    std::unordered_map<Node*, std::unordered_set<Node*>> m_adjacencyList;

    bool m_isDragging{false};
    bool m_isSelecting{false};

    double m_currentZoomScale{1.};
    bool m_editingEnabled{true};

    static constexpr double k_minScale = 1.;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.25;
};

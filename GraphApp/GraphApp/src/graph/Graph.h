#pragma once

#include "node/Node.h"
#include "edge/Edge.h"

class Graph : public QGraphicsView {
    Q_OBJECT

   public:
    Graph(QWidget* parent);
    ~Graph();

    void onAdjacencyListChanged(const QString& text);
    Node* getFirstSelectedNode() const;

    void genericTraversal(Node* start);
    void genericTotalTraversal(Node* start);
    void path(Node* start);
    void breadthFirstSearch(Node* start);
    void depthFirstSearch(Node* start);

    size_t getNodesCount();
    int getZoomPercentage();

    void resetGraph();

   signals:
    void zoomChanged();

   protected:
    void wheelEvent(QWheelEvent* event) final;

    void mousePressEvent(QMouseEvent* event) final;
    void mouseReleaseEvent(QMouseEvent* event) final;

    void keyPressEvent(QKeyEvent* event) final;

   private:
    void addNode(const QPointF& pos);
    void removeNode(Node* node);
    void removeSelectedNodes();

    void addEdge(Node* a, Node* b, int cost);
    void removeEdgesConnectedToNode(Node* node);

    void resetAdjacencyList();

    void markNodesAsUnvisited();
    void unmarkNodes();

    bool openStepDurationDialog();
    void waitForStepDuration();

    std::vector<Node*> m_nodes;
    std::vector<Edge*> m_edges;

    std::unordered_map<Node*, std::unordered_set<Node*>> m_adjacencyList;

    bool m_isDragging{false};
    bool m_isSelecting{false};

    double m_currentZoomScale{1.};

    int m_stepDuration{1000};

    std::mt19937 m_randomEngine;

    static constexpr double k_minScale = 1.;
    static constexpr double k_maxScale = 5.;
    static constexpr double k_zoomStep = 0.25;
};

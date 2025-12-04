#pragma once

class Graph;

class AlgorithmText : public QGraphicsTextItem {
    Q_OBJECT

   public:
    AlgorithmText(Graph* graph, int order, int additionalSpacing = 0);
    ~AlgorithmText();

    int getOrder() const;

    void setComputeFunction(std::function<void(AlgorithmText*)> computeFunction);
    void compute();

   private:
    void repositionAll();

    Graph* m_graph;
    int m_order;
    int m_additionalSpacing;

    std::function<void(AlgorithmText*)> m_computeFunction;
};

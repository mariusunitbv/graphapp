#pragma once

class Graph;

class AlgorithmText : public QGraphicsTextItem {
    Q_OBJECT

   public:
    AlgorithmText(Graph* graph, short order, int additionalSpacing = 0);
    ~AlgorithmText();

    short getOrder() const;

    void setComputeFunction(std::function<void(AlgorithmText*)> computeFunction);
    void compute();

    void hide();

   private:
    void repositionAll();

    Graph* m_graph;
    short m_order;
    int m_additionalSpacing;
    bool m_computedAtLeastOnce{false};

    std::function<void(AlgorithmText*)> m_computeFunction;
};

#pragma once

#include "../algorithm_label/algorithm_label.h"

class Node;

struct TraversalData {
    enum NodeState : uint8_t { UNVISITED, VISITED, ANALYZED };

    TraversalData(const std::vector<Node*>& nodes, QGraphicsView* view);
    virtual ~TraversalData() = default;

    virtual bool finished() = 0;

    const std::vector<Node*>* m_allNodes;

    std::vector<size_t> m_parents;
    AlgorithmLabel m_parentsLabel;

    std::vector<NodeState> m_visited;
    AlgorithmLabel m_unvisitedLabel;
    AlgorithmLabel m_visitedLabel;
    AlgorithmLabel m_analyzedLabel;

    QGraphicsView* m_view;
};

struct GenericTraversal : public TraversalData {
    GenericTraversal(const std::vector<Node*>& nodes, QGraphicsView* view);

    bool finished() override;

    std::vector<size_t> m_orders;
    AlgorithmLabel m_ordersLabel;

    std::vector<Node*> m_nodesVector;

    size_t m_currentIndex{0};
    size_t m_order{1};
};

struct TotalGenericTraversal : public GenericTraversal {
    TotalGenericTraversal(const std::vector<Node*>& nodes, QGraphicsView* view);

    bool finished() override;

    void pickAnotherNode();
};

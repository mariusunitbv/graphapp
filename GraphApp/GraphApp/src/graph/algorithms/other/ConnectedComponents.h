#pragma once

#include "../traversals/TraversalBase.h"

class ConnectedComponents : public TraversalBase {
    Q_OBJECT

   public:
    ConnectedComponents(Graph* parent);

    void showPseudocode() override;

   private:
    void setStartNode(Node* node) override;

    bool stepOnce() override;
    void stepAll() override;

    std::stack<Node*> m_nodesStack;
    Node* m_nodeToUnmarkFromBeingAnalyzed{nullptr};

    size_t m_connectedComponentsCount{0};
    AlgorithmText m_connectedComponentsCountLabel;

    std::vector<Node*> m_connectedComponentNodes;
    AlgorithmText m_connectedComponentsLabel;
};

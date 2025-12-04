#pragma once

#include "TraversalBase.h"

class GenericTraversal : public TraversalBase {
    Q_OBJECT

   public:
    GenericTraversal(Graph* parent);

    void setStartNode(Node* node) override;
    void showPseudocode() override;

   protected:
    bool stepOnce() override;
    void stepAll() override;

    /*
        In a generic traversal algorithm the nodes usually
        don't have any special order of how they are chosen.
    */
    std::vector<Node*> m_nodesVector;
    size_t m_currentNode{0};

    std::vector<size_t> m_discoveryOrder;
    size_t m_discoveryCount{0};
    AlgorithmText m_discoveryLabel;

    bool m_isTotalTraversal{false};
};

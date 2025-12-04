#pragma once

#include "TraversalBase.h"

class BreadthTraversal : public TraversalBase {
    Q_OBJECT

   public:
    BreadthTraversal(Graph* parent);

    void setStartNode(Node* node) override;
    void showPseudocode() override;

   private:
    bool stepOnce() override;
    void stepAll() override;

    std::queue<Node*> m_nodesQueue;

    std::vector<size_t> m_lengths;
    AlgorithmText m_lengthsLabel;
};

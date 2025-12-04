#pragma once

#include "TraversalBase.h"

class DepthTraversal : public TraversalBase {
    Q_OBJECT

   public:
    DepthTraversal(Graph* graph);

    void setStartNode(Node* node) override;
    void showPseudocode() override;

   private:
    bool stepOnce() override;
    void stepAll() override;

    std::stack<Node*> m_nodesStack;
    Node* m_nodeToUnmarkFromBeingAnalyzed{nullptr};

    std::vector<size_t> m_discoveryTimes;
    AlgorithmText m_discoveryTimesLabel;

    std::vector<size_t> m_analyzeTimes;
    AlgorithmText m_analyzeTimesLabel;

    size_t m_time{0};
};

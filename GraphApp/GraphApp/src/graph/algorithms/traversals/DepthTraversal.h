#pragma once

#include "TraversalBase.h"

class DepthTraversal : public TraversalBase {
    Q_OBJECT

   public:
    DepthTraversal(Graph* graph);

    void setStartNode(Node* node) override;
    void showPseudocode() override;

    const std::vector<size_t>& getAnalyzeTimes() const;

   protected:
    bool stepOnce() override;
    void stepAll() override;

    void updateEdgesClassificationOfNode(Node* x);
    void updateAllEdgesClassification();

    std::stack<Node*> m_nodesStack;
    Node* m_nodeToUnmarkFromBeingAnalyzed{nullptr};

    std::vector<size_t> m_discoveryTimes;
    AlgorithmText m_discoveryTimesLabel;

    std::vector<size_t> m_analyzeTimes;
    AlgorithmText m_analyzeTimesLabel;

    size_t m_time{0};

    std::vector<std::pair<size_t, size_t>> m_treeEdges;
    AlgorithmText m_treeEdgesLabel;

    std::vector<std::pair<size_t, size_t>> m_forwardEdges;
    AlgorithmText m_forwardEdgesLabel;

    std::vector<std::pair<size_t, size_t>> m_backEdges;
    AlgorithmText m_backEdgesLabel;

    std::vector<std::pair<size_t, size_t>> m_crossEdges;
    AlgorithmText m_crossEdgesLabel;

    bool m_isTotalTraversal{false};
};

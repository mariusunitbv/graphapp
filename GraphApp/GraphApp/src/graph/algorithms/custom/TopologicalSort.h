#pragma once

#include "../traversals/DepthFirstTotalTraversal.h"

class TopologicalSort final : public DepthFirstTotalTraversal {
    Q_OBJECT

   public:
    TopologicalSort(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void onFinishedAlgorithm() override;
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    void onPickedNewStartNode(NodeIndex_t startNode);
    void onVisitedNode(NodeIndex_t node);
    void onAnalyzingNode(NodeIndex_t node);
    void onAnalyzedNode(NodeIndex_t node);

    std::deque<NodeIndex_t> m_topologicalOrder;

    static constexpr auto BACK_EDGE = -1;
};

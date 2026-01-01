#pragma once

#include "../traversals/DepthFirstTotalTraversal.h"

class StronglyConnectedComponents : public ITimedAlgorithm {
    Q_OBJECT

   public:
    class CustomDepthFirstTraversal : public DepthFirstTraversal {
       public:
        friend class StronglyConnectedComponents;

        CustomDepthFirstTraversal(Graph* graph);

        bool step() override;

        void initializeForSCC(const std::vector<DFSInfo>& nodesInfo);

       private:
        bool pickNewStartNode();

        void onNodeAnalyzed(NodeIndex_t node);

        std::vector<NodeIndex_t> m_sortedNodesByAnalyzeTime;
        std::vector<NodeIndex_t> m_currentStronglyConnectedComponent;
        std::vector<std::vector<NodeIndex_t>> m_stronglyConnectedComponents;
    };

    StronglyConnectedComponents(Graph* graph);

    void start() override;
    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;

    void onFirstTraversalFinished();
    void onSecondTraversalFinished();
    void colorStronglyConnectedComponent(const std::vector<NodeIndex_t>& component);

    DepthFirstTotalTraversal* m_depthTraversal{nullptr};
    CustomDepthFirstTraversal* m_invertedDepthTraversal{nullptr};
    size_t m_currentComponentIndex{0};
};

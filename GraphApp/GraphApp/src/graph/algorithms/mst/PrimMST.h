#pragma once

#include "../ITimedAlgorithm.h"

class PrimMST : public ITimedAlgorithm {
    Q_OBJECT

   public:
    PrimMST(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    void pickLowestCostNode();

    struct PrimNodeInfo {
        void setCost(CostType_t cost) {
            m_minimalCost = cost;
            m_minimalCostInitialized = true;
        }

        NodeIndex_t m_parent{INVALID_NODE};
        CostType_t m_minimalCost{std::numeric_limits<CostType_t>::max()};
        bool m_inMST : 1 {false};
        bool m_minimalCostInitialized : 1 {false};
    };

    std::vector<PrimNodeInfo> m_nodeInfo;
    NodeIndex_t m_currentNode{INVALID_NODE};

    static constexpr auto MST_EDGE = 0;
};

#pragma once

#include "../ITimedAlgorithm.h"

#include "../utils/DisjointSet.h"

class BoruvkaMST : public ITimedAlgorithm {
    Q_OBJECT

   public:
    BoruvkaMST(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    struct BoruvkaMSTComponentInfo {
        std::vector<NodeIndex_t> m_nodes;
    };

    std::vector<BoruvkaMSTComponentInfo> m_components;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_mstEdges;
    std::unique_ptr<DisjointSet> m_disjointSet;
    bool m_shouldPickEdges{true};

    static constexpr auto MST_EDGE = 0;
};

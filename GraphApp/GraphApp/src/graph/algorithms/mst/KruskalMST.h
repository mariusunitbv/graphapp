#pragma once

#include "../ITimedAlgorithm.h"

#include "../utils/DisjointSet.h"

class KruskalMST : public ITimedAlgorithm {
    Q_OBJECT

   public:
    KruskalMST(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    void sortEdgesByCost();

    std::unique_ptr<DisjointSet> m_disjointSet;
    std::vector<std::tuple<CostType_t, NodeIndex_t, NodeIndex_t>> m_sortedEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_mstEdges;
    size_t m_currentEdgeIndex{0};

    static constexpr auto MST_EDGE = 0;
};

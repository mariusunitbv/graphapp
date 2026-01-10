#pragma once

#include "../ITimedAlgorithm.h"

#include "../utils/DisjointSet.h"

class GenericMST : public ITimedAlgorithm {
    Q_OBJECT

   public:
    GenericMST(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;

    void pickRandomNonEmptyComponent();
    void deselectCurrentComponent();

    struct GenericMSTComponentInfo {
        std::vector<NodeIndex_t> m_nodes;
        std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_edges;
    };

    std::vector<GenericMSTComponentInfo> m_components;
    std::unique_ptr<DisjointSet> m_disjointSet;
    size_t m_currentIteration{0};
    size_t m_currentComponentIndex{INVALID_COMPONENT};

    static constexpr auto MST_EDGE = 0;
    static constexpr auto INVALID_COMPONENT = std::numeric_limits<size_t>::max();
};

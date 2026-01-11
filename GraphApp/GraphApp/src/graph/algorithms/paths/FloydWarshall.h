#pragma once

#include "../ITimedAlgorithm.h"

class FloydWarshall : public ITimedAlgorithm {
    Q_OBJECT

   public:
    friend class FloydWarshallPath;

    FloydWarshall(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void colorNodesForCurrentStep();
    void uncolorPreviousNodes();
    void runParallelized();

    std::vector<int64_t> m_distanceMatrix;
    std::vector<NodeIndex_t> m_parentMatrix;

    NodeIndex_t m_currentK{0}, m_prevK{INVALID_NODE};
    NodeIndex_t m_currentI{0}, m_prevI{INVALID_NODE};
    NodeIndex_t m_currentJ{0}, m_prevJ{INVALID_NODE};
    bool m_firstStep{true}, m_negativeLoopCycle{false};

    static constexpr auto SHORTEST_PATH = 0;
    static constexpr auto CURRENT_PATH = 1;
    static constexpr auto VISITED_PATH = 2;

    static constexpr auto MAX_COST = std::numeric_limits<int64_t>::max();
};

#pragma once

#include "../ITimedAlgorithm.h"

class Dijkstra : public ITimedAlgorithm {
    Q_OBJECT

   public:
    Dijkstra(Graph* graph);

    void start(NodeIndex_t startNode, NodeIndex_t targetNode = INVALID_NODE);
    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;
    void markTargetUnreachable();
    void markPartialArborescence();

    NodeIndex_t m_startNode{INVALID_NODE};
    NodeIndex_t m_targetNode{INVALID_NODE};

    struct DijkstraNodeInfo {
        int64_t m_minCost{std::numeric_limits<int64_t>::max()};
        NodeIndex_t m_parent{INVALID_NODE};
    };

    std::vector<DijkstraNodeInfo> m_nodesInfo;

    using MinHeapEntry_t = std::pair<int64_t, NodeIndex_t>;
    std::priority_queue<MinHeapEntry_t, std::vector<MinHeapEntry_t>, std::greater<>> m_minHeap;

    static constexpr auto PATH_TO_TARGET = 0;
    static constexpr auto SHORTEST_PATHS = 1;
    static constexpr auto VISITED_PATH = 2;
};

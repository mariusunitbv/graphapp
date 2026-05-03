module;
#include <pch.h>

export module dijkstra;

export import algorithm_base;

export class Dijkstra : public AlgorithmBase {
   public:
    AlgorithmType getType() const override;
    const char* getName() const override;

    void initialize() override;

    void restartAlgorithm() override;
    bool stepAlgorithm() override;

    ExecutionInfo_t getExecutionInfo() const override;

   protected:
    void markTargetUnreachable();
    void markShortestPath();

    struct DijkstraNodeInfo {
        int64_t m_minCost{std::numeric_limits<int64_t>::max()};
        NodeIndex_t m_parent{INVALID_NODE};
    };

    std::vector<DijkstraNodeInfo> m_nodesInfo;
    NodeIndex_t m_currentMinNode{INVALID_NODE};

    uint32_t m_neighbourIndex{0};
    NodeIndex_t m_lastRelaxedNeighbour{INVALID_NODE};

    using MinHeapEntry_t = std::pair<int64_t, NodeIndex_t>;
    std::priority_queue<MinHeapEntry_t, std::vector<MinHeapEntry_t>, std::greater<>> m_minHeap;
};

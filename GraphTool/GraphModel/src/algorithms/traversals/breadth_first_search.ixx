module;
#include <pch.h>

export module breadth_first_search;

export import algorithm_base;

export class BreadthFirstSearch : public AlgorithmBase {
   public:
    AlgorithmType getType() const override;
    const char* getName() const override;

    void initialize() override;

    void restartAlgorithm() override;
    bool stepAlgorithm() override;

    ExecutionInfo_t getExecutionInfo() const override;

   protected:
    struct BFSNodeInfo {
        NodeIndex_t m_parent{INVALID_NODE};
        uint32_t m_distance{std::numeric_limits<uint32_t>::max()};
    };

    std::deque<NodeIndex_t> m_queue{};
    std::vector<BFSNodeInfo> m_nodesInfo{};
};

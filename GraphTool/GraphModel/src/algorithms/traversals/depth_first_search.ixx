module;
#include <pch.h>

export module depth_first_search;

export import algorithm_base;

export class DepthFirstSearch : public AlgorithmBase {
   public:
    AlgorithmType getType() const override;
    const char* getName() const override;

    void initialize() override;
    void restartAlgorithm() override;
    bool stepAlgorithm() override;

    ExecutionInfo_t getExecutionInfo() const override;

   protected:
    struct DFSNodeInfo {
        NodeIndex_t m_parent{INVALID_NODE};
        uint32_t m_visitTime{std::numeric_limits<uint32_t>::max()};
        uint32_t m_analyzeTime{std::numeric_limits<uint32_t>::max()};
    };

    std::deque<NodeIndex_t> m_stack{};
    std::vector<DFSNodeInfo> m_nodesInfo{};
    uint32_t m_time{1};
};

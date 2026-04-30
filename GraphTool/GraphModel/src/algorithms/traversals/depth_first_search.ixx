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
    void updateEdgesTypes(NodeIndex_t nodeIndex);

    struct DFSNodeInfo {
        NodeIndex_t m_parent{INVALID_NODE};
        uint32_t m_visitTime{std::numeric_limits<uint32_t>::max()};
        uint32_t m_analyzeTime{std::numeric_limits<uint32_t>::max()};
    };

    std::deque<NodeIndex_t> m_stack{};
    std::vector<DFSNodeInfo> m_nodesInfo{};
    uint32_t m_time{1};

    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_treeEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_forwardEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_backEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_crossEdges;

    NodeIndex_t m_nodeToStopAnalyzing{INVALID_NODE};
};

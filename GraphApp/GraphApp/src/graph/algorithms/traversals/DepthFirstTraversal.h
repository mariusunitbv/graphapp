#pragma once

#include "../ITimedAlgorithm.h"

class DepthFirstTraversal : public ITimedAlgorithm {
    Q_OBJECT

   public:
    friend class StronglyConnectedComponents;

    DepthFirstTraversal(Graph* graph);

    void start(NodeIndex_t startNode);
    bool step() override;
    void showPseudocodeForm() override;

   protected:
    void setStartNode(NodeIndex_t startNode);
    void onFinishedAlgorithm() override;
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    void updateEdgeClassification(NodeIndex_t node);

    struct DFSInfo {
        NodeIndex_t m_parentNode{INVALID_NODE};
        uint32_t m_discoveryTime{MAX_TIME};
        uint32_t m_analyzeTime{MAX_TIME};
    };

    std::vector<DFSInfo> m_nodesInfo;
    std::deque<NodeIndex_t> m_traversalContainer;
    uint32_t m_currentTime{0};

    NodeIndex_t m_startNode{INVALID_NODE};
    bool m_isTotalTraversal{false};

    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_treeEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_forwardEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_backEdges;
    std::vector<std::pair<NodeIndex_t, NodeIndex_t>> m_crossEdges;

    static constexpr auto ANALYZED_EDGE = 0;
    static constexpr auto VISITED_EDGE = 1;

    static constexpr auto MAX_TIME = std::numeric_limits<uint32_t>::max();
};

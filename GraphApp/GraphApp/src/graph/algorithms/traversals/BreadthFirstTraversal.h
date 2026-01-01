#pragma once

#include "../ITimedAlgorithm.h"

class BreadthFirstTraversal : public ITimedAlgorithm {
    Q_OBJECT

   public:
    BreadthFirstTraversal(Graph* graph);

    void start(NodeIndex_t startNode);
    bool step() override;
    void showPseudocodeForm() override;

   protected:
    void setStartNode(NodeIndex_t startNode);
    void onFinishedAlgorithm() override;
    void updateAlgorithmInfoText() const override;

    struct BFSInfo {
        NodeIndex_t m_parentNode{INVALID_NODE};
        uint32_t m_length{std::numeric_limits<uint32_t>::max()};
    };

    std::vector<BFSInfo> m_nodesInfo;
    std::deque<NodeIndex_t> m_traversalContainer;

    static constexpr auto ANALYZED_EDGE = 0;
    static constexpr auto ANALYZING_EDGE = 1;
    static constexpr auto VISITED_EDGE = 2;
};

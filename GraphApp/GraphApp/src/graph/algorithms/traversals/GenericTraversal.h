#pragma once

#include "../ITimedAlgorithm.h"

class GenericTraversal : public ITimedAlgorithm {
    Q_OBJECT

   public:
    friend class PathReconstruction;

    GenericTraversal(Graph* graph);

    void start(NodeIndex_t startNode);
    bool step() override;
    void showPseudocodeForm() override;

   protected:
    void setStartNode(NodeIndex_t startNode);
    void onFinishedAlgorithm() override;
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

    struct GenericInfo {
        struct Comparator {
            bool operator()(GenericInfo* a, GenericInfo* b) const {
                return a->m_randomVisitOrder > b->m_randomVisitOrder;
            }
        };

        NodeIndex_t m_parentNode{INVALID_NODE};
        uint32_t m_visitOrder{0};
        uint32_t m_randomVisitOrder{0};
    };

    std::vector<GenericInfo> m_nodesInfo;
    std::priority_queue<GenericInfo*, std::vector<GenericInfo*>, GenericInfo::Comparator>
        m_traversalContainer;

    uint32_t m_currentVisitOrder{0};

    NodeIndex_t m_startNode{INVALID_NODE};
    NodeIndex_t m_currentNode{INVALID_NODE};
    bool m_shouldMarkLastNodeVisited{false};
    bool m_isTotalTraversal{false};

    static constexpr auto ANALYZED_EDGE = 0;
    static constexpr auto ANALYZING_EDGE = 1;
    static constexpr auto VISITED_EDGE = 2;
};

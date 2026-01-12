#pragma once

#include "FloydWarshall.h"

class FloydWarshallPath : public ITimedAlgorithm {
    Q_OBJECT

   public:
    FloydWarshallPath(Graph* graph);

    void start(NodeIndex_t start, NodeIndex_t end);
    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
    void onEnterPressed();

    FloydWarshall* m_floydWarshallAlgorithm{nullptr};

    NodeIndex_t m_startNodeIndex{INVALID_NODE};
    NodeIndex_t m_endNodeIndex{INVALID_NODE};
    NodeIndex_t m_currentNodeIndex{INVALID_NODE};
    int64_t m_totalPathCost{0};
};

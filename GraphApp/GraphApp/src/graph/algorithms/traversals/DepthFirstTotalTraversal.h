#pragma once

#include "DepthFirstTraversal.h"

class DepthFirstTotalTraversal : public DepthFirstTraversal {
    Q_OBJECT

   public:
    DepthFirstTotalTraversal(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   protected:
    bool pickNewStartNode();
    void resetForUndo() override;

    size_t m_currentRandomIndex{0};
    std::vector<NodeIndex_t> m_randomOrderNodes;
};

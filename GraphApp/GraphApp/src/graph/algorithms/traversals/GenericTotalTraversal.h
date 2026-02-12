#pragma once

#include "GenericTraversal.h"

class GenericTotalTraversal : public GenericTraversal {
    Q_OBJECT

   public:
    GenericTotalTraversal(Graph* graph);

    bool step() override;
    void showPseudocodeForm() override;

   private:
    bool pickNewStartNode();
    void resetForUndo() override;

    size_t m_currentRandomIndex{0};
    std::vector<NodeIndex_t> m_randomOrderNodes;
};

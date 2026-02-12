#pragma once

#include "../traversals/GenericTraversal.h"

class PathReconstruction : public ITimedAlgorithm {
    Q_OBJECT

   public:
    PathReconstruction(Graph* graph);

    void start(NodeIndex_t start, NodeIndex_t end);
    bool step() override;
    void showPseudocodeForm() override;

   protected:
    void updateAlgorithmInfoText() const override;
    void resetForUndo() override;

   private:
    void setDefaultColorToVisitedNodes();
    void setDefaultColorForUndo();

    GenericTraversal* m_genericTraversal{nullptr};

    NodeIndex_t m_endNode{INVALID_NODE};
    NodeIndex_t m_currentNode{INVALID_NODE};

    bool m_firstStep{true};
};

#pragma once

#include "../traversals/GenericTraversal.h"

class PathReconstruction : public ITimedAlgorithm {
    Q_OBJECT

   public:
    PathReconstruction(Graph* graph);

    void start(NodeIndex_t start);
    bool step() override;
    void showPseudocodeForm() override;

   protected:
    void updateAlgorithmInfoText() const override;

   private:
    void setDefaultColorToVisitedNodes();

    GenericTraversal* m_genericTraversal{nullptr};
    NodeIndex_t m_currentNode{INVALID_NODE}, m_latestMarkedNode{INVALID_NODE};

    bool m_firstStep{true};
};

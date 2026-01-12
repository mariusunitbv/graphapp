#pragma once

#include "../ITimedAlgorithm.h"

class FordFulkerson : public ITimedAlgorithm {
    Q_OBJECT

   public:
    FordFulkerson(Graph* graph);

    void start(NodeIndex_t start, NodeIndex_t end);
    bool step() override;
    void showPseudocodeForm() override;

   private:
    void updateAlgorithmInfoText() const override;
};

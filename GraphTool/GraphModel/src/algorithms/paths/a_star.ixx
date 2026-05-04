module;
#include <pch.h>

export module a_star;

import dijkstra;

export class AStar : public Dijkstra {
   public:
    AlgorithmType getType() const override;
    const char* getName() const override;

   protected:
    int calculateHeuristic(NodeIndex_t node) const override;
};

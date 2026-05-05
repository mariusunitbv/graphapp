module;
#include <pch.h>

export module a_star_landmark;

import dijkstra;

export class AStarLandmark : public Dijkstra {
   public:
    AlgorithmType getType() const override;
    const char* getName() const override;

    void initialize() override;

   protected:
    int calculateHeuristic(NodeIndex_t node) const override;

    void addToATL(NodeIndex_t node);

    std::vector<std::vector<int>> m_atl;
};

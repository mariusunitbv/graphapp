module;
#include <pch.h>

export module heuristic;

export import graph_model_defines;

export enum class HeuristicType : uint8_t {
    NONE = 0,
    HAVERSINE = 1,
};

export class IHeuristic {
   public:
    virtual ~IHeuristic() = default;

    virtual HeuristicType getType() const = 0;
    virtual double distance(const Node* a, const Node* b) const = 0;
};

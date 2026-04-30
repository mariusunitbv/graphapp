module;
#include <pch.h>

export module algorithm_listener;

export import node_states;

export class IAlgorithmListener {
   public:
    virtual ~IAlgorithmListener() = default;

    virtual void onAlgorithmFinish() = 0;
    virtual void onAlgorithmPseudocodeEvent(const std::string_view event) = 0;

    virtual void onNodeStateChange(NodeIndex_t nodeIndex, NodeState newState) = 0;
};

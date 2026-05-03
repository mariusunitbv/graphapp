module;
#include <pch.h>

export module node_states;

export import graph_model_defines;

export enum NodeState : uint8_t {
    NONE = 0,

    // Traversals
    VISITED = 1,
    ANALYZING,
    ANALYZED,

    RELAXED,
    UNREACHABLE,

    MAX_NODE_STATE,
};

static_assert(MAX_NODE_STATE <= (1 << NODE_STATE_BITS),
              "NodeState values must fit within the allocated bits");

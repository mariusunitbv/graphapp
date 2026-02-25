module;
#include <pch.h>

export module graph_model_defines;

export import graph_math;

export using NodeIndex_t = uint32_t;

export constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max();

export constexpr auto NODE_LIMIT = 1'000'000'000;
export constexpr auto NODE_RADIUS = 28.f;
export constexpr auto NODE_DIAMETER = NODE_RADIUS * 2.f;

export constexpr auto WORLD_BOUNDS_FIXED_SIZE = 500000.f;
export constexpr BoundingBox2D WORLD_BOUNDS{-WORLD_BOUNDS_FIXED_SIZE, -WORLD_BOUNDS_FIXED_SIZE,
                                            WORLD_BOUNDS_FIXED_SIZE, WORLD_BOUNDS_FIXED_SIZE};

export struct Node {
    Node() = default;
    explicit Node(NodeIndex_t index, Vector2D worldPos);

    bool hasCustomColor() const;
    uint32_t getABGR() const;

    Vector2D m_worldPos{};
    NodeIndex_t m_index{0};

    uint8_t m_red{0}, m_green{0}, m_blue{0};

    // This will be used for algorithms that need to mark nodes as visited without needing extra
    // memory, it can be used as a bitfield for different purposes, but currently it's reserved.
   private:
    uint8_t m_reserved{0};
};

static_assert(sizeof(Node) == 16, "Node struct must be 16 bytes.");

export struct VisibleNode {
    VisibleNode() = default;
    explicit VisibleNode(Vector2D worldPos, NodeIndex_t index)
        : m_worldPos{worldPos}, m_index{index} {}

    Vector2D m_worldPos;
    uint32_t m_color;
    uint32_t m_outlineColor;

    NodeIndex_t m_index;
};

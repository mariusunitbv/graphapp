module;
#include <pch.h>

export module graph_model_defines;

import math;

export using NodeIndex_t = uint32_t;

export constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max();
export constexpr auto NODE_LIMIT = 500'000'000;
export constexpr auto NODE_RADIUS = 28.f;
export constexpr auto NODE_DIAMETER = NODE_RADIUS * 2.f;

export struct Node {
    Node() = default;
    explicit Node(NodeIndex_t index, Vector2D worldPos);

    void setIndex(NodeIndex_t index);

    bool hasColor() const;
    uint32_t getABGR() const;

    Vector2D m_worldPos{};
    NodeIndex_t m_index{0};
    char m_labelBuffer[10]{};

    uint8_t m_red{0}, m_green{0}, m_blue{0}, m_alpha{255};
};

export struct VisibleNode {
    Vector2D m_worldPos;
    uint32_t m_color;
    NodeIndex_t m_index;
};

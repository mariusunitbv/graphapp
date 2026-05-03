module;
#include <pch.h>

export module graph_view_model_defines;

import graph_model_defines;
import algorithm;

export struct GraphCamera {
    Vector2D m_position{};
    float m_zoom{1.0f};
};

export struct NodeColorInfo {
    uint32_t m_color;
    uint32_t m_outlineColor;
};

export struct VisibleEdge {
    explicit VisibleEdge(uint32_t startNodeLookup, uint32_t endNodeLookup, bool bothWay)
        : m_startNodeIndexLookup(startNodeLookup),
          m_endNodeIndexLookupAndBothWayFlag(endNodeLookup | (bothWay ? (1u << 31) : 0u)) {}

    uint32_t m_startNodeIndexLookup;
    uint32_t m_endNodeIndexLookupAndBothWayFlag;
};

export struct VisibleData {
    std::vector<Vector2D> m_nodesPositions;
    std::vector<NodeColorInfo> m_nodesColors;

    std::vector<NodeIndex_t> m_visibleNodes;
    std::vector<VisibleEdge> m_visibleEdges;

    // Bits indicating self-loops visibility for each node in m_visibleNodes.
    std::vector<uint8_t> m_visibleLoops;
};

export constexpr std::array<std::string_view, (size_t)AlgorithmType::ALGORITHM_TYPE_MAX>
    g_algorithmPseudocodes = {
        "assets/pseudocode/bfs.txt",
        "assets/pseudocode/dfs.txt",
        "assets/pseudocode/dijkstra.txt",
};

module;
#include <pch.h>

export module gridmap;

import graph_model_defines;

export class GridMap {
   public:
    void insert(const Node* node, NodeIndex_t nodeIndex);
    void remove(const std::vector<NodeIndex_t>& indexRemap);

    void incrementNodeCountInCells(const BoundingBox2D& nodeArea);
    void reserveNodeCountInCells();

    NodeIndex_t querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                            NodeIndex_t nodeToIgnore = INVALID_NODE) const;
    NodeIndex_t querySingleFast(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                                NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    template <typename Func>
    void visitNodes(std::span<const Node> nodes, const BoundingBox2D& area, Func&& func,
                    int maxNodesPerCell = -1) const;

    const BoundingBox2D& getBounds() const;
    void setBounds(const BoundingBox2D& bounds);

    bool isAllocated() const;
    void allocateCells();

   private:
    struct EntryCell {
        int m_minCellX{0};
        int m_maxCellX{0};
        int m_minCellY{0};
        int m_maxCellY{0};
    };

    EntryCell calculateCellEntryForNode(const BoundingBox2D& nodeArea) const;
    EntryCell calculateCellEntryForArea(const BoundingBox2D& area) const;

    int m_cellCountX{0};
    int m_cellCountY{0};

    BoundingBox2D m_bounds{};
    std::vector<std::vector<NodeIndex_t>> m_cells{};
    std::vector<uint32_t> m_nodeCountInCell{};
};

template <typename Func>
void GridMap::visitNodes(std::span<const Node> nodes, const BoundingBox2D& area, Func&& func,
                         int maxNodesPerCell) const {
    const auto cellEntry = calculateCellEntryForArea(area);
    for (int cellY = cellEntry.m_minCellY; cellY <= cellEntry.m_maxCellY; ++cellY) {
        for (int cellX = cellEntry.m_minCellX; cellX <= cellEntry.m_maxCellX; ++cellX) {
            const auto& cell = m_cells[cellY * m_cellCountX + cellX];

            int nodesInCell = static_cast<int>(cell.size());
            int step = 1;
            if (maxNodesPerCell > 0 && nodesInCell > maxNodesPerCell) {
                step = (nodesInCell + maxNodesPerCell - 1) / maxNodesPerCell;
            }

            for (int i = 0; i < nodesInCell; i += step) {
                const auto nodeIndex = cell[i];
                const auto& node = nodes[nodeIndex];
                if (area.intersects(Node::getBoundingBox(node.getWorldPos()))) {
                    if constexpr (std::is_invocable_r_v<bool, Func, NodeIndex_t>) {
                        if (!func(nodeIndex)) {
                            return;
                        }
                    } else {
                        func(nodeIndex);
                    }
                }
            }
        }
    }
}

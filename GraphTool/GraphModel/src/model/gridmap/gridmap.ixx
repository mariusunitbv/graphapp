module;
#include <pch.h>

export module gridmap;

import graph_model_defines;

export class GridMap {
   public:
    void insert(const Node* node, NodeIndex_t nodeIndex);
    void remove(const std::vector<NodeIndex_t>& indexRemap);

    void incrementNodeCountInCells(Vector2D nodePos);
    void reserveNodeCountInCells();

    NodeIndex_t querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                            NodeIndex_t nodeToIgnore = INVALID_NODE) const;
    NodeIndex_t querySingleFast(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                                NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    uint32_t estimateNodeCountInArea(const BoundingBox2D& area) const;

    template <typename Func>
    void visitNodes(std::span<const Node> nodes, const BoundingBox2D& area, Func&& func,
                    float percentage) const;

    const BoundingBox2D& getBounds() const;
    void setBounds(const BoundingBox2D& bounds);

    bool isAllocated() const;
    void allocateCells();

    static constexpr auto CELL_SIZE = 500;

   private:
    struct EntryCell {
        int m_minCellX{0};
        int m_maxCellX{0};
        int m_minCellY{0};
        int m_maxCellY{0};
    };

    EntryCell calculateCellEntryForNode(Vector2D nodePos) const;
    EntryCell calculateCellEntryForArea(const BoundingBox2D& area) const;

    int m_cellCountX{0};
    int m_cellCountY{0};

    BoundingBox2D m_bounds{};
    std::vector<std::vector<NodeIndex_t>> m_cells{};
    std::vector<uint32_t> m_nodeCountInCell{};
};

template <typename Func>
void GridMap::visitNodes(std::span<const Node> nodes, const BoundingBox2D& area, Func&& func,
                         float percentage) const {
    const auto cellEntry = calculateCellEntryForArea(area);
    for (int cellY = cellEntry.m_minCellY; cellY <= cellEntry.m_maxCellY; ++cellY) {
        for (int cellX = cellEntry.m_minCellX; cellX <= cellEntry.m_maxCellX; ++cellX) {
            const auto& cell = m_cells[cellY * m_cellCountX + cellX];

            int nodesInCell = static_cast<int>(cell.size());
            int maxNodesPerCell =
                std::max(1, static_cast<int>(std::ceil(nodesInCell * percentage)));

            const auto limit = std::min(nodesInCell, maxNodesPerCell);
            for (int i = 0; i < limit; ++i) {
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

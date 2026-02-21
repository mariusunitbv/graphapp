module;
#include <pch.h>

export module gridmap;

import graph_model_defines;
import math;

export class GridMap {
   public:
    void insert(const Node* node);
    void remove(NodeIndex_t toRemoveIndex, const BoundingBox2D& nodeArea);

    void fixIndexesAfterNodeRemoval(const std::vector<NodeIndex_t>& indexRemap);

    std::vector<VisibleNode> query(std::span<const Node> nodes, const BoundingBox2D& area) const;
    NodeIndex_t querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                            NodeIndex_t nodeToIgnore = INVALID_NODE) const;
    NodeIndex_t querySingleFast(std::span<const Node> nodes, Vector2D point,
                                const BoundingBox2D& area, float minimumDistance,
                                NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    const BoundingBox2D& getBounds() const;
    void setBounds(const BoundingBox2D& bounds);

    void allocateCells();

   private:
    struct EntryCell {
        int m_minCellX{0};
        int m_maxCellX{0};
        int m_minCellY{0};
        int m_maxCellY{0};
    };

    EntryCell calculateEntryCell(const BoundingBox2D& nodeArea) const;

    void query(std::span<const Node> nodes, const BoundingBox2D& area, std::vector<bool>& visitMask,
               std::vector<VisibleNode>& result) const;

    int m_cellCountX{0};
    int m_cellCountY{0};

    BoundingBox2D m_bounds{};
    std::vector<std::vector<NodeIndex_t>> m_cells{};
};

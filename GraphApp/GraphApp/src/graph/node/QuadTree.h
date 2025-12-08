#pragma once

#include "Node.h"

class QuadTree {
   public:
    ~QuadTree();

    void setBoundary(const QRect& boundary);
    const QRect& getBoundary() const;

    void insert(const NodeData& node);

    QuadTree* getContainingQuadTree(const NodeData& node);

    // NON-RECURSIVE METHODS
    bool needsReinserting(const NodeData& node);
    void remove(const NodeData& node);
    void update(const NodeData& node);

    void subdivide();
    bool isSubdivided() const;

    QuadTree* getNorthWest() const;
    QuadTree* getNorthEast() const;
    QuadTree* getSouthWest() const;
    QuadTree* getSouthEast() const;

    bool intersectsAnotherNode(QPoint pos, NodeIndex_t indexToIgnore = -1) const;
    std::optional<NodeIndex_t> getNodeAtPosition(QPoint pos, float minDistance,
                                                 NodeIndex_t indexToIgnore = -1) const;

   private:
    struct TreeNode {
        NodeIndex_t m_index;
        QPoint m_position;
    };

    QRect m_boundary{};

    QuadTree* m_northWest{nullptr};
    QuadTree* m_northEast{nullptr};
    QuadTree* m_southWest{nullptr};
    QuadTree* m_southEast{nullptr};

    static constexpr size_t k_maxCapacity{4};

    std::array<TreeNode, k_maxCapacity> m_nodes{};
    NodeIndex_t m_nodesCount{0};
};

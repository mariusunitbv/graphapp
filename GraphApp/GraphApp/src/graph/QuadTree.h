#pragma once

#include "Node.h"

class QuadTree {
   public:
    ~QuadTree();

    void setBoundary(const QRect& boundary);
    const QRect& getBoundary() const;

    void insert(const NodeData& node);
    void getContainingTrees(const NodeData& node, std::vector<QuadTree*>& trees);
    void getNodesInArea(const QRect& area, std::vector<bool>& visitMask,
                        std::vector<NodeIndex_t>& nodes) const;

    // NON-RECURSIVE METHODS
    bool needsReinserting(const NodeData& node) const;
    void update(const NodeData& node);
    void remove(const NodeData& node);
    void clear();

    void subdivide();
    bool isSubdivided() const;
    bool canSubdivide() const;

    QuadTree* getNorthWest() const;
    QuadTree* getNorthEast() const;
    QuadTree* getSouthWest() const;
    QuadTree* getSouthEast() const;

    bool intersectsAnotherNode(QPoint pos, float minDistance, NodeIndex_t indexToIgnore = -1) const;
    std::optional<NodeIndex_t> getNodeAtPosition(QPoint pos, float minDistance,
                                                 NodeIndex_t indexToIgnore = -1) const;

   private:
    struct TreeNode {
        TreeNode(NodeIndex_t index, QPoint position) : m_index(index), m_position(position) {}

        NodeIndex_t m_index;
        QPoint m_position;
    };

    std::optional<std::pair<NodeIndex_t, uint64_t>> getClosestNodeHelper(
        QPoint pos, uint64_t minDistanceSquared, NodeIndex_t indexToIgnore) const;

    QRect m_boundary{};

    QuadTree* m_northWest{nullptr};
    QuadTree* m_northEast{nullptr};
    QuadTree* m_southWest{nullptr};
    QuadTree* m_southEast{nullptr};

    std::vector<TreeNode> m_nodes{};

    static constexpr auto k_maxSoftCapacity{8};
};

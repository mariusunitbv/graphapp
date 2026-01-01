#pragma once

#include "Node.h"

using QuadTreePtr_t = std::unique_ptr<class QuadTree>;

class QuadTree {
   public:
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

    const QuadTreePtr_t& getNorthWest() const;
    const QuadTreePtr_t& getNorthEast() const;
    const QuadTreePtr_t& getSouthWest() const;
    const QuadTreePtr_t& getSouthEast() const;

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

    QuadTreePtr_t m_northWest{};
    QuadTreePtr_t m_northEast{};
    QuadTreePtr_t m_southWest{};
    QuadTreePtr_t m_southEast{};

    std::vector<TreeNode> m_nodes{};

    static constexpr auto k_maxSoftCapacity{8};
};

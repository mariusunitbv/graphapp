module;
#include <pch.h>

export module quadtree;

import graph_model_defines;
import math;

export class QuadTree {
   public:
    QuadTree();

    void insert(const Node* node);
    void remove(NodeIndex_t toRemoveIndex, const BoundingBox2D& nodeArea);
    void clear();

    void fixIndexesAfterNodeRemoval(const std::vector<NodeIndex_t>& indexRemap);

    std::vector<VisibleNode> query(std::span<const Node> nodes, const BoundingBox2D& area) const;
    NodeIndex_t querySingle(std::span<const Node> nodes, Vector2D point, float minimumDistance,
                            NodeIndex_t nodeToIgnore = INVALID_NODE) const;
    NodeIndex_t querySingleFast(std::span<const Node> nodes, Vector2D point,
                                const BoundingBox2D& area, float minimumDistance,
                                NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    bool isSubdivided() const;
    bool canSubdivide() const;
    void subdivide();

    const BoundingBox2D& getBounds() const;
    BoundingBox2D& getBoundsMutable();
    void setBounds(const BoundingBox2D& bounds);
    bool validBounds() const;

    const QuadTree* getTopLeft() const { return m_topLeft.get(); }
    const QuadTree* getTopRight() const { return m_topRight.get(); }
    const QuadTree* getBottomLeft() const { return m_bottomLeft.get(); }
    const QuadTree* getBottomRight() const { return m_bottomRight.get(); }

   private:
    void query(std::span<const Node> nodes, const BoundingBox2D& area, std::vector<bool>& visitMask,
               std::vector<VisibleNode>& result) const;
    void querySingle(std::span<const Node> nodes, Vector2D point, const BoundingBox2D& area,
                     float& minimumDistanceSquared, NodeIndex_t nodeToIgnore,
                     NodeIndex_t& closestNodeIndex) const;

    BoundingBox2D m_bounds{};
    std::vector<NodeIndex_t> m_nodes;

    std::unique_ptr<QuadTree> m_topLeft;
    std::unique_ptr<QuadTree> m_topRight;
    std::unique_ptr<QuadTree> m_bottomLeft;
    std::unique_ptr<QuadTree> m_bottomRight;
};

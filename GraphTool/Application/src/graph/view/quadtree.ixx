module;
#include <pch.h>

export module quadtree;

import graph_model;

export using QuadTreePtr_t = std::unique_ptr<class QuadTree>;

export class QuadTree {
   public:
    QuadTree();

    void insert(const Node* node);
    void remove(GraphModel* graphModel, const std::unordered_set<NodeIndex_t>& nodes);
    void remove(NodeIndex_t toRemoveIndex, const ImVec4& nodeArea);
    void clear();

    void fixIndexesAfterNodeRemoval(GraphModel* graphModel);

    std::vector<NodeIndex_t> query(GraphModel* graphModel, const ImVec4& area,
                                   size_t totalNodeCount) const;
    NodeIndex_t querySingle(GraphModel* graphModel, const ImVec2& point, float minimumDistance,
                            NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    bool isSubdivided() const;
    bool canSubdivide() const;
    void subdivide();

    const ImVec4& getBounds() const;
    void setBounds(float left, float top, float right, float bottom);
    bool validBounds() const;

    QuadTree* getTopLeft() { return m_topLeft.get(); }
    QuadTree* getTopRight() { return m_topRight.get(); }
    QuadTree* getBottomLeft() { return m_bottomLeft.get(); }
    QuadTree* getBottomRight() { return m_bottomRight.get(); }

   private:
    void query(GraphModel* graphModel, const ImVec4& area, std::vector<bool>& visitMask,
               std::vector<NodeIndex_t>& result) const;
    void querySingle(GraphModel* graphModel, const ImVec2& point, const ImVec4& area,
                     float& minimumDistanceSquared, NodeIndex_t nodeToIgnore,
                     NodeIndex_t& closestNodeIndex) const;

    ImVec4 m_bounds{};
    std::vector<NodeIndex_t> m_nodes;

    QuadTreePtr_t m_topLeft;
    QuadTreePtr_t m_topRight;
    QuadTreePtr_t m_bottomLeft;
    QuadTreePtr_t m_bottomRight;

    static constexpr auto SOFT_MAX_NODES_PER_QUAD = 8;
};

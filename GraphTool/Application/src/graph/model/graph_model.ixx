module;
#include <pch.h>

export module graph_model;

export import utils;
export import graph_model_defines;

import math;
import quadtree;

export class GraphModel {
   public:
    void addNode(Vector2D worldPos);
    void removeNodes(const std::unordered_set<NodeIndex_t>& nodes);

    void reserveNodes(size_t nodeCount);

    void beginBulkInsert();
    void endBulkInsert();

    Node* getNode(NodeIndex_t index);
    const Node* getNode(NodeIndex_t index) const;
    const std::vector<Node>& getNodes() const;

    NodeIndex_t getNodeIndexAfterRemoval(NodeIndex_t originalIndex) const;

    const QuadTree* getQuadTree() const;

    static BoundingBox2D getNodeBoundingBox(Vector2D worldPos);

   private:
    bool updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds);
    void removeNodesAndCalculateIndexRemap(const std::unordered_set<NodeIndex_t>& nodes);

    void rebuildQuadTree();

    std::vector<Node> m_nodes;
    std::vector<NodeIndex_t> m_indexRemapAfterRemoval;

    QuadTree m_quadTree;

    bool m_bulkInsertMode{false};
};

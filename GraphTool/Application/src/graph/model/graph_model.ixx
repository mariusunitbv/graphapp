module;
#include <pch.h>

export module graph_model;

export import graph_model_defines;
export import quadtree;
export import utils;
export import math;

export class GraphModel {
   public:
    void addNode(Vector2D worldPos);
    void removeNodes(const std::unordered_set<NodeIndex_t>& nodes);

    void reserveNodes(size_t nodeCount);

    void beginBulkInsert();
    void endBulkInsert();

    NodeIndex_t getLastNodeIndex() const;

    Node* getNode(NodeIndex_t index);
    const Node* getNode(NodeIndex_t index) const;

    Node* getNodeAtPosition(Vector2D worldPos, bool firstOccurence = false,
                            float minimumDistance = NODE_RADIUS,
                            NodeIndex_t nodeToIgnore = INVALID_NODE);
    const Node* getNodeAtPosition(Vector2D worldPos, bool firstOccurence = false,
                                  float minimumDistance = NODE_RADIUS,
                                  NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    const QuadTree* getQuadTree() const;
    std::vector<VisibleNode> queryNodes(const BoundingBox2D& area) const;

    static BoundingBox2D getNodeBoundingBox(Vector2D worldPos);

   private:
    bool updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds);

    void removeNodesFromQuadTree(const std::unordered_set<NodeIndex_t>& nodes);
    std::vector<NodeIndex_t> removeNodesAndCalculateIndexRemap(
        const std::unordered_set<NodeIndex_t>& nodes);

    void rebuildQuadTree();

    std::vector<Node> m_nodes;
    QuadTree m_quadTree;

    bool m_bulkInsertMode{false};
};

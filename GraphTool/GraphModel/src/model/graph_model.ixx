module;
#include <pch.h>

export module graph_model;

export import graph_model_defines;
export import gridmap;

export class GraphModel {
   public:
    void addNode(Vector2D worldPos);
    void removeSelectedNodes();

    void reserveNodes(size_t nodeCount);
    void reserveArea(const BoundingBox2D& area);

    void beginBulkInsert();
    void endBulkInsert();

    NodeIndex_t getLastNodeIndex() const;
    NodeIndex_t getNodeIndex(const Node* node) const;

    Node* getNode(NodeIndex_t index);
    const Node* getNode(NodeIndex_t index) const;

    Node* getNodeAtPosition(Vector2D worldPos, bool firstOccurence = false,
                            float minimumDistance = NODE_RADIUS,
                            NodeIndex_t nodeToIgnore = INVALID_NODE);
    const Node* getNodeAtPosition(Vector2D worldPos, bool firstOccurence = false,
                                  float minimumDistance = NODE_RADIUS,
                                  NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    const BoundingBox2D& getGraphBounds() const;

    template <typename Func>
    void visitNodes(const BoundingBox2D& area, Func&& func, int maxNodesPerCell = -1) const;

   private:
    bool updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds);
    void rebuildGridMap();

    std::vector<NodeIndex_t> removeSelectedNodesAndCalculateIndexRemap();

    std::vector<Node> m_nodes;
    GridMap m_gridMap;

    bool m_bulkInsertMode{false};
};

template <typename Func>
void GraphModel::visitNodes(const BoundingBox2D& area, Func&& func, int maxNodesPerCell) const {
    static_assert(std::is_invocable_v<Func, NodeIndex_t>,
                  "visitNodes: callback must accept a single NodeIndex_t parameter");

    m_gridMap.visitNodes(m_nodes, area, std::forward<Func>(func), maxNodesPerCell);
}

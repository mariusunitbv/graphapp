module;
#include <pch.h>

export module graph_model;

export import graph_model_defines;

import gridmap;
import edge_storage;

export class GraphModel {
   public:
    GraphModel();

    static constexpr int getGridMapCellSize();

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
    uint32_t estimateNodeCountInArea(const BoundingBox2D& area) const;

    template <typename Func>
    void visitNodes(const BoundingBox2D& area, Func&& func, float percentage = 1.f) const;

    void addEdge(NodeIndex_t src, NodeIndex_t dest, int weight);
    void addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight);
    void removeEdge(NodeIndex_t src, NodeIndex_t dest);

    void sortEdges();

    size_t getNodeDegree(NodeIndex_t index) const;
    void visitNeighbours(NodeIndex_t src, void* userData,
                         void (*callback)(void* userData, NodeIndex_t dest, int weight),
                         float percentage = 1.f, bool distinct = false) const;

    void resizeEdgeStorage(size_t nodeCount);

   private:
    bool updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds);
    void rebuildGridMap();

    std::vector<NodeIndex_t> removeSelectedNodesAndCalculateIndexRemap();

    std::vector<Node> m_nodes;
    GridMap m_gridMap;
    std::unique_ptr<EdgeStorage> m_edgeStorage;

    bool m_bulkInsertMode{false};
};

template <typename Func>
void GraphModel::visitNodes(const BoundingBox2D& area, Func&& func, float percentage) const {
    static_assert(std::is_invocable_v<Func, NodeIndex_t>,
                  "visitNodes: callback must accept a single NodeIndex_t parameter");

    m_gridMap.visitNodes(m_nodes, area, std::forward<Func>(func), percentage);
}

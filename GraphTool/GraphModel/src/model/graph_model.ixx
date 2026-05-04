module;
#include <pch.h>

export module graph_model;

export import graph_model_defines;

import gridmap;
import edge_storage;
import graph_common;
import heuristic;

export class GraphModel {
   public:
    GraphModel();

    int getGridMapCellSize() const;

    void addNode(Vector2D worldPos);
    void removeSelectedNodes();

    void reserveNodes(uint32_t nodeCount);
    void reserveArea(const BoundingBox2D& area);

    void beginBulkInsert();
    void endBulkInsert();

    void setMetadata(const std::string& key, const std::string& value);
    const std::unordered_map<std::string, std::string>& getMetadata() const;

    void setHeuristic(HeuristicType type);
    double heuristicDistance(NodeIndex_t a, NodeIndex_t b) const;
    HeuristicType getHeuristicType() const;
    bool hasHeuristic() const;

    uint32_t getNodeCount() const;
    NodeIndex_t getLastNodeIndex() const;
    NodeIndex_t getNodeIndex(const Node* node) const;

    Node* getNode(NodeIndex_t index);
    const Node* getNode(NodeIndex_t index) const;

    Node* getNodeAtPosition(Vector2D worldPos, float minimumDistance, bool firstOccurence = false,
                            NodeIndex_t nodeToIgnore = INVALID_NODE);
    const Node* getNodeAtPosition(Vector2D worldPos, float minimumDistance,
                                  bool firstOccurence = false,
                                  NodeIndex_t nodeToIgnore = INVALID_NODE) const;

    const BoundingBox2D& getGraphBounds() const;
    uint32_t estimateNodeCountInArea(const BoundingBox2D& area) const;

    template <typename Func>
    void visitNodes(const BoundingBox2D& area, Func&& func, float nodesRadius,
                    float percentage = 1.f) const;

    void addEdge(NodeIndex_t src, NodeIndex_t dest, int weight);
    void addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight);
    void removeEdge(NodeIndex_t src, NodeIndex_t dest);
    int getEdgeWeight(NodeIndex_t src, NodeIndex_t dest, int defaultValue = -1) const;

    void reserveDegree(NodeIndex_t nodeIndex, uint32_t degree);
    void sortEdges();

    uint32_t getNodeDegree(NodeIndex_t index) const;
    std::span<const EdgeStorage::Edge_t> getNodeEdges(NodeIndex_t index) const;

    void visitNeighbours(NodeIndex_t src, void* userData,
                         bool (*callback)(void* userData, NodeIndex_t dest, int weight),
                         float percentage = 1.f) const;
    void visitDistinctNeighbours(NodeIndex_t src, void* userData,
                                 bool (*callback)(void* userData, NodeIndex_t dest, int weight,
                                                  bool bothWays),
                                 float percentage = 1.f) const;

    void resizeEdgeStorage(uint32_t nodeCount);

   private:
    bool updateDynamicBoundsIfNeeded(const BoundingBox2D& bounds);
    void rebuildGridMap();

    common::MediumVector<NodeIndex_t> removeSelectedNodesAndCalculateIndexRemap();

    common::MediumVector<Node> m_nodes;
    GridMap m_gridMap;

    std::unique_ptr<EdgeStorage> m_edgeStorage;
    std::unique_ptr<IHeuristic> m_heuristic;

    std::unordered_map<std::string, std::string> m_metadata;

    bool m_bulkInsertMode{false};
};

template <typename Func>
void GraphModel::visitNodes(const BoundingBox2D& area, Func&& func, float nodesRadius,
                            float percentage) const {
    static_assert(std::is_invocable_v<Func, NodeIndex_t>,
                  "visitNodes: callback must accept a single NodeIndex_t parameter");

    m_gridMap.visitNodes(m_nodes.span(), area, std::forward<Func>(func), nodesRadius, percentage);
}

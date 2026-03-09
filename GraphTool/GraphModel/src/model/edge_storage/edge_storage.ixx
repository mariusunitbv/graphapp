module;
#include <pch.h>

export module edge_storage;

export import graph_model_defines;

export class EdgeStorage {
   public:
    // NodeIndex_t is the index of the target node, and int is the edge weight.
    using Edge_t = std::pair<NodeIndex_t, int>;

    virtual ~EdgeStorage() = default;

    virtual void resize(size_t nodeCount) = 0;
    virtual void onNodeAdded(NodeIndex_t nodeIndex) = 0;

    virtual void addEdge(NodeIndex_t src, NodeIndex_t dest, int weight) = 0;
    virtual void addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) = 0;
    virtual bool hasEdge(NodeIndex_t src, NodeIndex_t dest, int* outWeight = nullptr) const = 0;
    virtual void removeEdge(NodeIndex_t src, NodeIndex_t dest) = 0;
    virtual void remove(const std::vector<NodeIndex_t>& indexRemap) = 0;

    virtual void sortEdges() = 0;

    virtual size_t getNeighbourCount(NodeIndex_t src) const = 0;
    virtual void visitNeighbours(NodeIndex_t src, void* userData,
                                 bool (*callback)(void* userData, NodeIndex_t dest, int weight),
                                 float percentage, bool distinct) const = 0;
};

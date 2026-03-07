module;
#include <pch.h>

export module editable_edge_storage;

export import edge_storage;

export class EditableEdgeStorage : public EdgeStorage {
   public:
    void resize(size_t nodeCount) override;
    void onNodeAdded(NodeIndex_t nodeIndex) override;

    void addEdge(NodeIndex_t src, NodeIndex_t dest, int weight) override;
    void addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) override;
    bool hasEdge(NodeIndex_t src, NodeIndex_t dest, int* outWeight = nullptr) const override;
    void removeEdge(NodeIndex_t src, NodeIndex_t dest) override;
    void remove(const std::vector<NodeIndex_t>& indexRemap) override;

    void sortEdges() override;

    size_t getNeighbourCount(NodeIndex_t src) const override;
    void visitNeighbours(NodeIndex_t src, void* userData,
                         void (*callback)(void* userData, NodeIndex_t dest, int weight),
                         float percentage, bool distinct) const override;

   private:
    std::vector<std::vector<Edge_t>> m_edges;
};

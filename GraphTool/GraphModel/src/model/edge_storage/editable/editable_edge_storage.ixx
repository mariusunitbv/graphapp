module;
#include <pch.h>

export module editable_edge_storage;

export import edge_storage;

import graph_common;

export class EditableEdgeStorage : public EdgeStorage {
   public:
    void resize(uint32_t nodeCount) override;
    void onNodeAdded(NodeIndex_t nodeIndex) override;

    void addEdge(NodeIndex_t src, NodeIndex_t dest, int weight) override;
    void addEdgeFast(NodeIndex_t src, NodeIndex_t dest, int weight) override;
    bool hasEdge(NodeIndex_t src, NodeIndex_t dest, int* outWeight = nullptr) const override;
    void removeEdge(NodeIndex_t src, NodeIndex_t dest) override;
    void remove(const common::MediumVector<NodeIndex_t>& indexRemap) override;

    void reserveDegree(NodeIndex_t nodeIndex, uint32_t degree) override;
    void sortEdges() override;

    uint32_t getNeighbourCount(NodeIndex_t src) const override;
    std::span<const Edge_t> getNeighbours(NodeIndex_t src) const override;

    void visitNeighbours(NodeIndex_t src, void* userData,
                         bool (*callback)(void* userData, NodeIndex_t dest, int weight),
                         float percentage, bool distinct) const override;
    void visitDistinctNeighbours(NodeIndex_t src, void* userData,
                                 bool (*callback)(void* userData, NodeIndex_t dest, int weight,
                                                  bool bothWays),
                                 float percentage) const override;

   private:
    common::MediumVector<common::TinyVector<Edge_t>> m_edges;
    bool m_edgesSorted{true};
};

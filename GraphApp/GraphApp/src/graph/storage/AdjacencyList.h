#pragma once

#include "IGraphStorage.h"

class AdjacencyList final : public IGraphStorage {
   public:
    using Neighbour_t = std::pair<NodeIndex_t, CostType_t>;
    using Neighbours_t = std::vector<Neighbour_t>;
    using AdjacencyList_t = std::vector<Neighbours_t>;

    Type type() const override;

    void resize(size_t nodeCount) override;

    void addEdge(NodeIndex_t start, NodeIndex_t end, CostType_t cost) override;
    void removeEdge(NodeIndex_t start, NodeIndex_t end) override;

    std::optional<CostType_t> getEdge(NodeIndex_t start, NodeIndex_t end) const override;

    void forEachOutgoingEdge(
        NodeIndex_t node,
        const std::function<void(NodeIndex_t, CostType_t)>& callback) const override;
    void forEachOutgoingEdgeWithOpposites(
        NodeIndex_t node,
        const std::function<void(NodeIndex_t, CostType_t)>& callback) const override;

    void recomputeBeforeRemovingNodes(
        size_t oldNodeCount,
        const std::set<NodeIndex_t, std::greater<NodeIndex_t>>& selectedNodes) override;
    void recomputeAfterAddingNode(size_t newNodeCount) override;

   private:
    Neighbours_t::iterator getNeighbour(NodeIndex_t start, NodeIndex_t end);
    Neighbours_t::const_iterator getNeighbour(NodeIndex_t start, NodeIndex_t end) const;

    AdjacencyList_t m_adjacencyList;
};

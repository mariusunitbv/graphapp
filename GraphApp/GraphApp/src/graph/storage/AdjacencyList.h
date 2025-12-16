#pragma once

#include "IGraphStorage.h"

#include "../tracker/MemoryTracker.h"

class AdjacencyList final : public IGraphStorage {
   public:
    using Neighbour_t = std::pair<NodeIndex_t, CostType_t>;
    using Neighbours_t = std::vector<Neighbour_t, TrackingAllocator<Neighbour_t>>;

    using AdjacencyListValue_t = std::pair<const NodeIndex_t, Neighbours_t>;
    using AdjacencyList_t =
        std::unordered_map<NodeIndex_t, Neighbours_t, std::hash<NodeIndex_t>,
                           std::equal_to<NodeIndex_t>, TrackingAllocator<AdjacencyListValue_t>>;

    explicit AdjacencyList();

    Type type() const override;

    void resize(size_t nodeCount) override;

    void addEdge(NodeIndex_t start, NodeIndex_t end, CostType_t cost) override;
    void removeEdge(NodeIndex_t start, NodeIndex_t end) override;

    bool hasEdge(NodeIndex_t start, NodeIndex_t end) const override;
    CostType_t getCost(NodeIndex_t start, NodeIndex_t end) const override;

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

    size_t getMemoryUsage() const override;

   private:
    Neighbours_t::iterator getNeighbour(NodeIndex_t start, NodeIndex_t end);
    Neighbours_t::const_iterator getNeighbour(NodeIndex_t start, NodeIndex_t end) const;

    MemoryTracker m_memoryTracker;
    AdjacencyList_t m_adjacencyList;
};

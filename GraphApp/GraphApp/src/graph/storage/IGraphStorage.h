#pragma once

#include "../Node.h"

using CostType_t = int16_t;

class IGraphStorage {
   public:
    enum class Type { ADJACENCY_LIST, ADJACENCY_MATRIX };

    virtual ~IGraphStorage() = default;

    virtual Type type() const = 0;

    virtual void resize(size_t nodeCount) = 0;

    virtual void addEdge(NodeIndex_t start, NodeIndex_t end, CostType_t cost) = 0;
    virtual void removeEdge(NodeIndex_t start, NodeIndex_t end) = 0;

    virtual bool hasEdge(NodeIndex_t start, NodeIndex_t end) const = 0;
    virtual CostType_t getCost(NodeIndex_t start, NodeIndex_t end) const = 0;

    virtual void forEachOutgoingEdge(
        NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const = 0;
    virtual void forEachOutgoingEdgeWithOpposites(
        NodeIndex_t node, const std::function<void(NodeIndex_t, CostType_t)>& callback) const = 0;

    virtual void recomputeBeforeRemovingNodes(
        size_t oldNodeCount,
        const std::set<NodeIndex_t, std::greater<NodeIndex_t>>& selectedNodes) = 0;
    virtual void recomputeAfterAddingNode(size_t newNodeCount) = 0;

    virtual size_t getMemoryUsage() const = 0;
};

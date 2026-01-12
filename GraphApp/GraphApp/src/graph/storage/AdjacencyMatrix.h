#pragma once

#include "IGraphStorage.h"

/**
 * @class AdjacencyMatrix
 * @brief Compact adjacency matrix for storing graph edges and costs.
 *
 * Storage Format:
 * Each matrix entry uses a bit-packed format for memory efficiency:
 * - MSB (leftmost bit): Edge existence flag (1 = edge exists, 0 = no edge)
 * - Remaining N-1 bits: Edge cost stored as signed integer in two's complement
 *
 * Example with int16_t (16 bits total):
 * - MSB (leftmost bit): Edge flag
 * - Next 15 bits: Cost value (range: -16384 to 16383)
 *
 * Binary representation: [Flag bit (MSB)][15-bit cost][LSB]
 *
 * This design allows querying edge existence and retrieving cost in a single
 * memory access, improving cache efficiency for graph algorithms.
 */
class AdjacencyMatrix final : public IGraphStorage {
   public:
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

    void complete();

   private:
    using UnsignedCostType_t = std::make_unsigned<CostType_t>::type;

    static constexpr UnsignedCostType_t FLAG_BIT = 1 << (sizeof(CostType_t) * 8 - 1);
    static constexpr UnsignedCostType_t COST_MASK = ~FLAG_BIT;

    static UnsignedCostType_t encode(CostType_t cost);
    UnsignedCostType_t read(NodeIndex_t i, NodeIndex_t j) const;

    size_t m_nodeCount{0};
    std::vector<UnsignedCostType_t> m_matrix{};
};

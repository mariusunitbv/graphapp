#pragma once

#include "Node.h"

using CostType_t = int8_t;

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
class AdjacencyMatrix {
   public:
    AdjacencyMatrix() = default;

    AdjacencyMatrix(const AdjacencyMatrix&) = delete;
    AdjacencyMatrix& operator=(const AdjacencyMatrix&) = delete;

    AdjacencyMatrix(AdjacencyMatrix&& rhs) noexcept;
    AdjacencyMatrix& operator=(AdjacencyMatrix&& rhs) noexcept;

    void resize(size_t nodeCount);
    bool empty() const;

    void reset();
    void complete();

    void setEdge(NodeIndex_t i, NodeIndex_t j, CostType_t cost);
    void clearEdge(NodeIndex_t i, NodeIndex_t j);

    bool hasEdge(NodeIndex_t i, NodeIndex_t j) const;
    CostType_t getCost(NodeIndex_t i, NodeIndex_t j) const;

   private:
    using UnsignedCostType_t = std::make_unsigned<CostType_t>::type;

    static constexpr UnsignedCostType_t FLAG_BIT = 1 << (sizeof(CostType_t) * 8 - 1);
    static constexpr UnsignedCostType_t COST_MASK = ~FLAG_BIT;

    static UnsignedCostType_t encode(CostType_t cost);
    UnsignedCostType_t read(NodeIndex_t i, NodeIndex_t j) const;

    NodeIndex_t m_nodeCount{0};
    std::vector<UnsignedCostType_t> m_matrix{};
};

static_assert(std::is_same_v<CostType_t, int8_t> || std::is_same_v<CostType_t, int16_t> ||
                  std::is_same_v<CostType_t, int32_t>,
              "CostType_t must be int8_t, int16_t, or int32_t");

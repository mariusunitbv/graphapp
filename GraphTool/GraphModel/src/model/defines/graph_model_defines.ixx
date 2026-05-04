module;
#include <pch.h>

export module graph_model_defines;

export import graph_math;

export using NodeIndex_t = uint32_t;

export constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max();
export constexpr auto NODE_LIMIT = 1'500'000'000;

export constexpr auto NODE_POS_BITS = 22;  // Number of bits allocated for each coordinate (X and Y)
export constexpr auto NODE_STATE_BITS = 3;

export constexpr auto WORLD_BOUNDS_FIXED_SIZE = 2'000'000.f;
export constexpr BoundingBox2D WORLD_BOUNDS{-WORLD_BOUNDS_FIXED_SIZE, -WORLD_BOUNDS_FIXED_SIZE,
                                            WORLD_BOUNDS_FIXED_SIZE, WORLD_BOUNDS_FIXED_SIZE};

// Node represents a single point in the graph with compact storage.
//
// Memory layout is carefully packed into 8 bytes (64 bits):
//  - 20 bits for m_worldPosX: fixed-range world position X
//  - 20 bits for m_worldPosY: fixed-range world position Y
//  - 7 bits each for m_red, m_green, m_blue: compressed RGB color
//  - 3 bits reserved for algorithmic flags (e.g., visited, state)
//
// Reasons for this packing:
//  1. Memory efficiency: each node occupies exactly 8 bytes, no padding.
//  2. Cache-friendly: contiguous arrays of nodes fit well in CPU cache.
//  3. Bitfields allow storing all necessary data (position, color, state) without wasting space.
export struct Node {
    Node() = default;
    explicit Node(Vector2D worldPos);

    // Helper function to get the bounding box of a node based on its world position, used for
    // spatial queries. It is calculated from the worldPos and the NODE_RADIUS, which is constant
    // for all nodes.
    static BoundingBox2D getBoundingBox(Vector2D worldPos, float radius);

    Vector2D getWorldPos() const;

    bool hasCustomColor() const;
    uint32_t getABGR(int alpha = 255) const;
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void clearColor();

    void setState(uint8_t state);
    uint8_t getState() const;

    void markSelected();
    void unmarkSelected();
    bool isSelected() const;

   private:
    uint64_t m_worldPosX : NODE_POS_BITS{};
    uint64_t m_worldPosY : NODE_POS_BITS{};

    // Color is stored in a compact 5-6-5 format (5 bits for red, 6 bits for green, 5 bits for
    // blue).
    uint64_t m_red : 5 {}, m_green : 6 {}, m_blue : 5 {};

    // This flag indicates whether the node is selected, which is way faster to check than using
    // contains() on a set of selected nodes.
    uint64_t m_selected : 1 {0};

    // Used by algorithms to store temporary state (e.g., visited, analyzed). The specific meaning
    // of the state bits is defined by the algorithm and should be reset before running a new
    // algorithm.
    uint64_t m_state : NODE_STATE_BITS{0};
};

static_assert(sizeof(Node) == 8, "Node struct must be 8 bytes.");

static_assert(WORLD_BOUNDS_FIXED_SIZE <= (1 << NODE_POS_BITS) / 2,
              "WORLD_BOUNDS_FIXED_SIZE must fit within the range defined by NODE_POS_BITS.");

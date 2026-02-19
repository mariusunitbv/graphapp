module;
#include <pch.h>

export module graph_model;

export import utils;

export using NodeIndex_t = uint32_t;

export constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max() - 1;
export constexpr auto NODE_LIMIT = 500'000'000;

export struct Node {
    Node() = default;

    explicit Node(NodeIndex_t index, float x, float y);

    void setIndex(NodeIndex_t index);
    bool hasColor() const;

    float m_x{0.f}, m_y{0.f};

    NodeIndex_t m_index{0};
    char m_labelBuffer[11]{};

    uint8_t m_red{0}, m_green{0}, m_blue{0};
};

export class GraphModel {
   public:
    using OnAddNodeCallback_t = Callback<const Node*>;

    void addNode(float x, float y);
    void removeNodes(const std::unordered_set<NodeIndex_t>& nodes);
    void setOnAddNodeCallback(void* instance, OnAddNodeCallback_t::Func_t callback);

    void reserveNodes(size_t nodeCount);

    const std::vector<Node>& getNodes() const;
    NodeIndex_t getNodeIndexAfterRemoval(NodeIndex_t originalIndex) const;

    void clearIndexRemap();

   private:
    std::vector<Node> m_nodes;
    std::vector<NodeIndex_t> m_indexRemapAfterRemoval;

    OnAddNodeCallback_t m_onAddNodeCallback{};
};

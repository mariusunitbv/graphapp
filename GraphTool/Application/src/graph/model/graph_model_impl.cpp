module;
#include <pch.h>

module graph_model;

Node::Node(NodeIndex_t index, float x, float y) : m_x(x), m_y(y) { setIndex(index); }

void Node::setIndex(NodeIndex_t index) {
    m_index = index;

    size_t charCount = 1;
    while (index >= 10) {
        index /= 10;
        ++charCount;
    }

    if (charCount > sizeof(m_labelBuffer) - 1) {
        GAPP_THROW("Node index exceeds label buffer size");
    }

    std::snprintf(m_labelBuffer, charCount + 1, "%u", m_index);
}

bool Node::hasColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

void GraphModel::addNode(float x, float y) {
    if (m_nodes.size() >= NODE_LIMIT) {
        GAPP_THROW("Node limit reached");
    }

    m_nodes.emplace_back(static_cast<NodeIndex_t>(m_nodes.size()), x, y);
    m_onAddNodeCallback(&m_nodes.back());
}

void GraphModel::removeNodes(const std::unordered_set<NodeIndex_t>& nodes) {
    m_indexRemapAfterRemoval.resize(m_nodes.size());

    NodeIndex_t writeIndex = 0;

    for (NodeIndex_t readIndex = 0; readIndex < m_nodes.size(); ++readIndex) {
        if (nodes.contains(m_nodes[readIndex].m_index)) {
            m_indexRemapAfterRemoval[readIndex] = INVALID_NODE;
            continue;
        }

        if (writeIndex != readIndex) {
            m_nodes[writeIndex] = std::move(m_nodes[readIndex]);
            m_nodes[writeIndex].setIndex(writeIndex);
        }

        m_indexRemapAfterRemoval[readIndex] = writeIndex;

        ++writeIndex;
    }

    m_nodes.resize(writeIndex);
}

void GraphModel::setOnAddNodeCallback(void* instance, OnAddNodeCallback_t::Func_t callback) {
    m_onAddNodeCallback.set(instance, callback);
}

void GraphModel::reserveNodes(size_t nodeCount) { m_nodes.reserve(nodeCount); }

const std::vector<Node>& GraphModel::getNodes() const { return m_nodes; }

NodeIndex_t GraphModel::getNodeIndexAfterRemoval(NodeIndex_t originalIndex) const {
    if (m_indexRemapAfterRemoval.empty()) {
        GAPP_THROW("No nodes have been removed, index remap is not available");
    }

    return m_indexRemapAfterRemoval[originalIndex];
}

void GraphModel::clearIndexRemap() {
    m_indexRemapAfterRemoval.clear();
    m_indexRemapAfterRemoval.shrink_to_fit();
}

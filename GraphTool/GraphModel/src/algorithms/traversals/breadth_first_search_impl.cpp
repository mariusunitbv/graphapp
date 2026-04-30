module;
#include <pch.h>

module breadth_first_search;

AlgorithmType BreadthFirstSearch::getType() const { return AlgorithmType::BREADTH_FIRST_SEARCH; }

const char* BreadthFirstSearch::getName() const { return "Breadth-First Search"; }

void BreadthFirstSearch::initialize() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    m_nodesInfo.resize(m_model->getNodeCount());

    m_queue.push_back(m_sourceNode);
    m_nodesInfo[m_sourceNode].m_distance = 0;
}

void BreadthFirstSearch::restartAlgorithm() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    m_queue = {};

    for (auto& nodeInfo : m_nodesInfo) {
        nodeInfo.m_parent = INVALID_NODE;
        nodeInfo.m_distance = std::numeric_limits<uint32_t>::max();
    }

    m_queue.push_back(m_sourceNode);
    m_nodesInfo[m_sourceNode].m_distance = 0;
}

bool BreadthFirstSearch::stepAlgorithm() {
    if (m_queue.empty()) {
        return false;
    }

    const auto x = m_queue.front();
    if (getNodeState(x) != NodeState::ANALYZING) {
        setNodeState(x, NodeState::ANALYZING);
        notifyPseudocodeEvent("analyzing");

        return true;
    }

    const auto& neigbours = m_model->getNodeEdges(x);
    for (const auto& [y, _] : neigbours) {
        if (getNodeState(y) != NodeState::NONE) {
            continue;
        }

        setNodeState(y, NodeState::VISITED);
        notifyPseudocodeEvent("visit");

        m_queue.push_back(y);

        m_nodesInfo[y].m_parent = x;
        m_nodesInfo[y].m_distance = m_nodesInfo[x].m_distance + 1;
    }

    m_queue.pop_front();
    setNodeState(x, NodeState::ANALYZED);
    notifyPseudocodeEvent("analyzed");

    return true;
}

IAlgorithm::ExecutionInfo_t BreadthFirstSearch::getExecutionInfo() const {
    ExecutionInfo_t info;

    info.emplace_back("U (Unvisited nodes)", "");
    info.emplace_back("V (Visited nodes)", "");
    info.emplace_back("W (Analyzed nodes)", "");
    info.emplace_back("p (Parent nodes)", "");
    info.emplace_back("l (Distances)", "");

    auto& U = info[0].second;
    auto& V = info[1].second;
    auto& W = info[2].second;
    auto& p = info[3].second;
    auto& l = info[4].second;

    uint32_t uCount = 0, vCount = 0, wCount = 0;
    uint32_t pCount = 0, lCount = 0;

    U = "U = {";
    V = "V = {";
    W = "W = {";
    p = "p = [";
    l = "l = [";

    for (NodeIndex_t i = 0; i < m_model->getNodeCount(); ++i) {
        const auto state = getNodeState(i);
        if (state == NodeState::NONE) {
            if (uCount++ > 0) {
                U += ", ";
            }

            if (uCount % 10 == 0) {
                U += '\n';
            }

            U += std::to_string(i);
        } else if (state == NodeState::ANALYZED) {
            if (wCount++ > 0) {
                W += ", ";
            }

            if (wCount % 10 == 0) {
                W += '\n';
            }

            W += std::to_string(i);
        }
    }

    for (const auto& nodeIndex : m_queue) {
        if (vCount++ > 0) {
            V += ", ";
        }

        if (vCount % 10 == 0) {
            V += '\n';
        }

        V += std::to_string(nodeIndex);
    }

    U += "}";
    V += "}";
    W += "}";

    for (const auto& nodeInfo : m_nodesInfo) {
        if (pCount++ > 0) {
            p += ", ";
        }
        if (pCount % 10 == 0) {
            p += '\n';
        }

        if (nodeInfo.m_parent == INVALID_NODE) {
            p += '-';
        } else {
            p += std::to_string(nodeInfo.m_parent);
        }

        if (lCount++ > 0) {
            l += ", ";
        }
        if (lCount % 10 == 0) {
            l += '\n';
        }

        if (nodeInfo.m_distance == std::numeric_limits<uint32_t>::max()) {
            l += "∞";
        } else {
            l += std::to_string(nodeInfo.m_distance);
        }
    }

    p += "]";
    l += "]";

    return info;
}

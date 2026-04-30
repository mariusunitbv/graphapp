module;
#include <pch.h>

module depth_first_search;

AlgorithmType DepthFirstSearch::getType() const { return AlgorithmType::DEPTH_FIRST_SEARCH; }

const char* DepthFirstSearch::getName() const { return "Depth First Search"; }

void DepthFirstSearch::initialize() {
    setNodesState(NodeState::NONE);

    m_nodesInfo.resize(m_model->getNodeCount());

    m_stack.push_back(m_sourceNode);
    m_nodesInfo[m_sourceNode].m_visitTime = m_time++;
}

void DepthFirstSearch::restartAlgorithm() {
    setNodesState(NodeState::NONE);

    m_stack = {};
    m_time = 0;

    for (auto& nodeInfo : m_nodesInfo) {
        nodeInfo.m_parent = INVALID_NODE;
        nodeInfo.m_visitTime = std::numeric_limits<uint32_t>::max();
        nodeInfo.m_analyzeTime = std::numeric_limits<uint32_t>::max();
    }

    m_stack.push_back(m_sourceNode);
    m_nodesInfo[m_sourceNode].m_visitTime = m_time++;
}

bool DepthFirstSearch::stepAlgorithm() {
    if (m_stack.empty()) {
        return false;
    }

    const auto x = m_stack.back();
    if (getNodeState(x) != NodeState::ANALYZING) {
        setNodeState(x, NodeState::ANALYZING);
        return true;
    }

    bool addedNode = false;
    const auto& neigbours = m_model->getNodeEdges(x);
    for (const auto& [y, _] : neigbours) {
        if (getNodeState(y) != NodeState::NONE) {
            continue;
        }

        setNodeState(y, NodeState::VISITED);
        m_stack.push_back(y);

        m_nodesInfo[y].m_parent = x;
        m_nodesInfo[y].m_visitTime = m_time++;
        addedNode = true;

        break;
    }

    if (!addedNode) {
        m_stack.pop_back();
        setNodeState(x, NodeState::ANALYZED);
        m_nodesInfo[x].m_analyzeTime = m_time++;
    } else {
        setNodeState(x, NodeState::VISITED);
        setNodeState(m_stack.back(), NodeState::ANALYZING);
    }

    return true;
}

IAlgorithm::ExecutionInfo_t DepthFirstSearch::getExecutionInfo() const {
    ExecutionInfo_t info;

    info.emplace_back("U (Unvisited nodes)", "");
    info.emplace_back("V (Visited nodes)", "");
    info.emplace_back("W (Analyzed nodes)", "");
    info.emplace_back("t (Current time)", "t = " + std::to_string(m_time));
    info.emplace_back("t1 (Visit time)", "");
    info.emplace_back("t2 (Analyze time)", "");
    info.emplace_back("p (Parent nodes)", "");

    auto& U = info[0].second;
    auto& V = info[1].second;
    auto& W = info[2].second;
    auto& t1 = info[4].second;
    auto& t2 = info[5].second;
    auto& p = info[6].second;

    uint32_t uCount = 0, vCount = 0, wCount = 0;
    uint32_t t1Count = 0, t2Count = 0, pCount = 0;

    U = "U = {";
    V = "V = {";
    W = "W = {";
    t1 = "t1 = [";
    t2 = "t2 = [";
    p = "p = [";

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

    for (const auto& nodeIndex : m_stack) {
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

        if (t1Count++ > 0) {
            t1 += ", ";
        }
        if (t1Count % 10 == 0) {
            t1 += '\n';
        }

        if (nodeInfo.m_visitTime == std::numeric_limits<uint32_t>::max()) {
            t1 += "∞";
        } else {
            t1 += std::to_string(nodeInfo.m_visitTime);
        }

        if (t2Count++ > 0) {
            t2 += ", ";
        }

        if (t2Count % 10 == 0) {
            t2 += '\n';
        }

        if (nodeInfo.m_analyzeTime == std::numeric_limits<uint32_t>::max()) {
            t2 += "∞";
        } else {
            t2 += std::to_string(nodeInfo.m_analyzeTime);
        }
    }

    p += "]";
    t1 += "]";
    t2 += "]";

    return info;
}

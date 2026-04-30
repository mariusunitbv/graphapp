module;
#include <pch.h>

module depth_first_search;

AlgorithmType DepthFirstSearch::getType() const { return AlgorithmType::DEPTH_FIRST_SEARCH; }

const char* DepthFirstSearch::getName() const { return "Depth First Search"; }

void DepthFirstSearch::initialize() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    m_nodesInfo.resize(m_model->getNodeCount());

    m_stack.push_back(m_sourceNode);
    m_nodesInfo[m_sourceNode].m_visitTime = m_time++;
    m_nodeToStopAnalyzing = INVALID_NODE;
}

void DepthFirstSearch::restartAlgorithm() {
    setNodesState(NodeState::NONE);
    notifyPseudocodeEvent("init");

    m_stack = {};
    m_time = 0;
    m_nodeToStopAnalyzing = INVALID_NODE;

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

    if (m_nodeToStopAnalyzing != INVALID_NODE) {
        setNodeState(m_nodeToStopAnalyzing, NodeState::VISITED);
        m_nodeToStopAnalyzing = INVALID_NODE;
    }

    const auto x = m_stack.back();
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

        m_stack.push_back(y);
        m_treeEdges.emplace_back(x, y);

        m_nodesInfo[y].m_parent = x;
        m_nodesInfo[y].m_visitTime = m_time++;
        m_nodeToStopAnalyzing = x;

        return true;
    }

    m_stack.pop_back();
    setNodeState(x, NodeState::ANALYZED);
    notifyPseudocodeEvent("analyzed");
    m_nodesInfo[x].m_analyzeTime = m_time++;
    updateEdgesTypes(x);

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
    info.emplace_back("P (Tree edges)", "");
    info.emplace_back("I (Forward edges)", "");
    info.emplace_back("R (Back edges)", "");
    info.emplace_back("T (Cross edges)", "");

    auto& U = info[0].second;
    auto& V = info[1].second;
    auto& W = info[2].second;
    auto& t1 = info[4].second;
    auto& t2 = info[5].second;
    auto& p = info[6].second;
    auto& P = info[7].second;
    auto& I = info[8].second;
    auto& R = info[9].second;
    auto& T = info[10].second;

    uint32_t uCount = 0, vCount = 0, wCount = 0;
    uint32_t t1Count = 0, t2Count = 0, pCount = 0;

    uint32_t PCount = 0, ICount = 0, RCount = 0, TCount = 0;

    U = "U = {";
    V = "V = {";
    W = "W = {";
    t1 = "t1 = [";
    t2 = "t2 = [";
    p = "p = [";
    P = "P = {";
    I = "I = {";
    R = "R = {";
    T = "T = {";

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

    for (auto [x, y] : m_treeEdges) {
        if (PCount++ > 0) {
            P += ", ";
        }

        if (PCount % 5 == 0) {
            P += '\n';
        }

        P += "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    for (auto [x, y] : m_forwardEdges) {
        if (ICount++ > 0) {
            I += ", ";
        }

        if (ICount % 5 == 0) {
            I += '\n';
        }

        I += "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    for (auto [x, y] : m_backEdges) {
        if (RCount++ > 0) {
            R += ", ";
        }

        if (RCount % 5 == 0) {
            R += '\n';
        }

        R += "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    for (auto [x, y] : m_crossEdges) {
        if (TCount++ > 0) {
            T += ", ";
        }

        if (TCount % 5 == 0) {
            T += '\n';
        }

        T += "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    P += "}";
    I += "}";
    R += "}";
    T += "}";

    return info;
}

void DepthFirstSearch::updateEdgesTypes(NodeIndex_t nodeIndex) {
    const auto& neighbours = m_model->getNodeEdges(nodeIndex);
    for (const auto& [neighbour, _] : neighbours) {
        if (getNodeState(neighbour) == NodeState::NONE) {
            continue;
        }

        if (nodeIndex == neighbour) {
            m_backEdges.emplace_back(nodeIndex, neighbour);
            continue;
        }

        if (m_nodesInfo[neighbour].m_parent == nodeIndex) {
            continue;
        }

        const auto t1X = m_nodesInfo[nodeIndex].m_visitTime;
        const auto t2X = m_nodesInfo[nodeIndex].m_analyzeTime;

        const auto t1Y = m_nodesInfo[neighbour].m_visitTime;
        const auto t2Y = m_nodesInfo[neighbour].m_analyzeTime;

        if (t1X < t1Y && t1Y < t2Y && t2Y < t2X) {
            m_forwardEdges.emplace_back(nodeIndex, neighbour);
        } else if (t1Y < t1X && t1X < t2X && t2X < t2Y) {
            m_backEdges.emplace_back(nodeIndex, neighbour);
        } else if (t1Y < t2Y && t2Y < t1X && t1X < t2X) {
            m_crossEdges.emplace_back(nodeIndex, neighbour);
        }
    }
}

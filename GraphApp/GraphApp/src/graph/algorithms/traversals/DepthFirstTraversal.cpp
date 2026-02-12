#include <pch.h>

#include "DepthFirstTraversal.h"

DepthFirstTraversal::DepthFirstTraversal(Graph* graph) : ITimedAlgorithm(graph) {
    auto& graphManager = graph->getGraphManager();
    const auto nodeCount = graphManager.getNodesCount();

    m_nodesInfo.resize(nodeCount);

    graphManager.setAlgorithmPathColor(ANALYZED_EDGE, qRgb(60, 179, 113));
    graphManager.setAlgorithmPathColor(VISITED_EDGE, qRgb(70, 130, 180));
}

void DepthFirstTraversal::start(NodeIndex_t startNode) {
    m_startNode = startNode;
    setStartNode(startNode);

    ITimedAlgorithm::start();
}

bool DepthFirstTraversal::step() {
    if (m_traversalContainer.empty()) {
        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    const auto currentNode = m_traversalContainer.back();

    if (getNodeState(currentNode) != NodeData::State::ANALYZING) {
        setNodeState(currentNode, NodeData::State::ANALYZING);
        m_pseudocodeForm.highlight({m_isTotalTraversal ? 11 : 9});
        emit analyzingNode(currentNode);

        return true;
    }

    bool addedNewNode = false;
    graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        currentNode, [&](NodeIndex_t neighbour, CostType_t) {
            if (addedNewNode) {
                return;
            }

            const auto neighbourState = getNodeState(neighbour);
            if (neighbourState == NodeData::State::UNVISITED) {
                setNodeState(neighbour, NodeData::State::VISITED);

                graphManager.addAlgorithmEdge(currentNode, neighbour, VISITED_EDGE);

                m_nodesInfo[neighbour].m_parentNode = currentNode;
                m_nodesInfo[neighbour].m_discoveryTime = ++m_currentTime;
                m_traversalContainer.push_back(neighbour);
                m_treeEdges.emplace_back(currentNode, neighbour);

                if (m_isTotalTraversal) {
                    m_pseudocodeForm.highlight({13, 14});
                } else {
                    m_pseudocodeForm.highlight({11, 12});
                }

                emit visitedNode(neighbour);
                addedNewNode = true;
            }
        });

    if (!addedNewNode) {
        setNodeState(currentNode, NodeData::State::ANALYZED);
        if (m_isTotalTraversal) {
            m_pseudocodeForm.highlight({15, 16});
        } else {
            m_pseudocodeForm.highlight({13, 14});
        }

        m_nodesInfo[currentNode].m_analyzeTime = ++m_currentTime;
        updateEdgeClassification(currentNode);
        emit analyzedNode(currentNode);

        m_traversalContainer.pop_back();
    } else {
        setNodeState(currentNode, NodeData::State::VISITED);
        setNodeState(m_traversalContainer.back(), NodeData::State::ANALYZING);
    }

    return true;
}

void DepthFirstTraversal::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PDF;
(2) BEGIN
(3)     U := N − {s}; V := {s}; W := ∅;
(4)     FOR toți y ∈ N DO p(y) := 0;
(5)     t := 1; t1(s) := 1; t2(s) := ∞;
(6)     FOR toți y ∈ U DO t1(y) := ∞; t2(y) := ∞;
(7)     WHILE V ≠ ∅ DO
(8)     BEGIN
(9)         se selectează cel mai nou nod x introdus în V;
(10)        IF există arc(x, y) ∈ A și y ∈ U
(11)        THEN U := U − {y}; V := V ∪ {y}; p(y) := x;
(12)             t := t + 1; t1(y) := t
(13)        ELSE V := V − {x}; W := W ∪ {x};
(14)             t := t + 1; t2(x) := t;
(15)     END;
(16) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void DepthFirstTraversal::setStartNode(NodeIndex_t startNode) {
    m_traversalContainer.push_back(startNode);
    m_nodesInfo[startNode].m_discoveryTime = ++m_currentTime;

    emit pickedStartNode(startNode);
}

void DepthFirstTraversal::onFinishedAlgorithm() {
    m_graph->getGraphManager().clearAlgorithmPath(VISITED_EDGE);

    ITimedAlgorithm::onFinishedAlgorithm();
}

void DepthFirstTraversal::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList U, V, W, p, t1, t2, P, I, R, T;
    for (auto node : m_traversalContainer) {
        V << QString::number(node);
    }

    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const auto state = getNodeState(nodeIndex);
        switch (state) {
            case NodeData::State::UNVISITED:
                U << QString::number(nodeIndex);
                break;
            case NodeData::State::ANALYZED:
                W << QString::number(nodeIndex);
                break;
        }

        if (m_nodesInfo[nodeIndex].m_parentNode == INVALID_NODE) {
            p << "-";
        } else {
            p << QString::number(m_nodesInfo[nodeIndex].m_parentNode);
        }

        if (m_nodesInfo[nodeIndex].m_discoveryTime == MAX_TIME) {
            t1 << "∞";
        } else {
            t1 << QString::number(m_nodesInfo[nodeIndex].m_discoveryTime);
        }

        if (m_nodesInfo[nodeIndex].m_analyzeTime == MAX_TIME) {
            t2 << "∞";
        } else {
            t2 << QString::number(m_nodesInfo[nodeIndex].m_analyzeTime);
        }
    }

    for (auto [x, y] : m_treeEdges) {
        P << QString("(%1, %2)").arg(x).arg(y);
    }

    for (auto [x, y] : m_forwardEdges) {
        I << QString("(%1, %2)").arg(x).arg(y);
    }

    for (auto [x, y] : m_backEdges) {
        R << QString("(%1, %2)").arg(x).arg(y);
    }

    for (auto [x, y] : m_crossEdges) {
        T << QString("(%1, %2)").arg(x).arg(y);
    }

    infoLines << "U = {" + U.join(", ") + "}";
    infoLines << "V = {" + V.join(", ") + "}";
    infoLines << "W = {" + W.join(", ") + "}";
    infoLines << "";
    infoLines << "p: [" + p.join(", ") + "]";
    infoLines << "";
    infoLines << "t1: [" + t1.join(", ") + "]";
    infoLines << "t2: [" + t2.join(", ") + "]";
    infoLines << "";
    infoLines << "P = {" + P.join(", ") + "}";
    infoLines << "I = {" + I.join(", ") + "}";
    infoLines << "R = {" + R.join(", ") + "}";
    infoLines << "T = {" + T.join(", ") + "}";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void DepthFirstTraversal::resetForUndo() {
    for (auto& info : m_nodesInfo) {
        info.m_parentNode = INVALID_NODE;
        info.m_discoveryTime = info.m_analyzeTime = MAX_TIME;
    }

    m_traversalContainer.clear();

    m_currentTime = 0;

    m_treeEdges.clear();
    m_forwardEdges.clear();
    m_backEdges.clear();
    m_crossEdges.clear();

    setStartNode(m_startNode);
}

void DepthFirstTraversal::updateEdgeClassification(NodeIndex_t node) {
    m_graph->getGraphManager().getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        node, [&](NodeIndex_t neighbour, CostType_t) {
            if (getNodeState(neighbour) == NodeData::State::UNVISITED) {
                return;
            }

            m_graph->getGraphManager().addAlgorithmEdge(node, neighbour, ANALYZED_EDGE);

            if (node == neighbour) {
                m_backEdges.emplace_back(node, node);
                return;
            }

            if (m_nodesInfo[neighbour].m_parentNode == node) {
                return;
            }

            const auto t1X = m_nodesInfo[node].m_discoveryTime;
            const auto t2X = m_nodesInfo[node].m_analyzeTime;

            const auto t1Y = m_nodesInfo[neighbour].m_discoveryTime;
            const auto t2Y = m_nodesInfo[neighbour].m_analyzeTime;

            if (t1X < t1Y && t1Y < t2Y && t2Y < t2X) {
                m_forwardEdges.emplace_back(node, neighbour);
            } else if (t1Y < t1X && t1X < t2X && t2X < t2Y) {
                m_backEdges.emplace_back(node, neighbour);
            } else if (t1Y < t2Y && t2Y < t1X && t1X < t2X) {
                m_crossEdges.emplace_back(node, neighbour);
            }
        });
}

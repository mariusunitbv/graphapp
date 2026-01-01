#include <pch.h>

#include "GenericTraversal.h"

#include "../random/Random.h"

GenericTraversal::GenericTraversal(Graph* graph) : ITimedAlgorithm(graph) {
    auto& graphManager = graph->getGraphManager();
    const auto nodeCount = graphManager.getNodesCount();

    m_nodesInfo.resize(nodeCount);
    m_traversalContainer.reserve(nodeCount);

    graphManager.setAlgorithmPathColor(ANALYZED_EDGE, qRgb(60, 179, 113));
    graphManager.setAlgorithmPathColor(ANALYZING_EDGE, qRgb(255, 165, 0));
    graphManager.setAlgorithmPathColor(VISITED_EDGE, qRgb(70, 130, 180));
}

void GenericTraversal::start(NodeIndex_t startNode) {
    setStartNode(startNode);

    ITimedAlgorithm::start();
}

bool GenericTraversal::step() {
    if (m_traversalContainer.empty()) {
        return false;
    }

    auto& graphManager = m_graph->getGraphManager();
    if (m_shouldMarkLastNodeVisited) {
        graphManager.clearAlgorithmPath(ANALYZING_EDGE);

        setNodeState(m_currentNode, NodeData::State::VISITED);

        m_currentNode = INVALID_NODE;
        m_shouldMarkLastNodeVisited = false;
        return true;
    }

    if (m_currentNode == INVALID_NODE) {
        pickRandomNodeFromTraversalContainer();
    }

    if (getNodeState(m_currentNode) != NodeData::State::ANALYZING) {
        setNodeState(m_currentNode, NodeData::State::ANALYZING);
        m_pseudocodeForm.highlight({m_isTotalTraversal ? 11 : 9});

        return true;
    }

    bool addedNewNode = false;
    graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        m_currentNode, [&](NodeIndex_t neighbour, CostType_t) {
            if (addedNewNode) {
                return;
            }

            const auto neighbourState = getNodeState(neighbour);
            if (neighbourState == NodeData::State::UNVISITED) {
                setNodeState(neighbour, NodeData::State::VISITED);

                graphManager.addAlgorithmEdge(m_currentNode, neighbour, ANALYZING_EDGE);
                graphManager.addAlgorithmEdge(m_currentNode, neighbour, VISITED_EDGE);

                m_nodesInfo[neighbour].m_parentNode = m_currentNode;
                m_nodesInfo[neighbour].m_visitOrder = ++m_currentVisitOrder;
                m_traversalContainer.push_back(neighbour);

                if (m_isTotalTraversal) {
                    m_pseudocodeForm.highlight({13, 14});
                } else {
                    m_pseudocodeForm.highlight({11, 12});
                }

                addedNewNode = true;
            }
        });

    if (!addedNewNode) {
        setNodeState(m_currentNode, NodeData::State::ANALYZED);
        m_pseudocodeForm.highlight({m_isTotalTraversal ? 15 : 13});

        graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
            m_currentNode, [&](NodeIndex_t neighbour, CostType_t) {
                if (getNodeState(neighbour) != NodeData::State::UNVISITED) {
                    graphManager.addAlgorithmEdge(m_currentNode, neighbour, ANALYZED_EDGE);
                }
            });

        m_traversalContainer.erase(m_traversalContainer.begin() + m_traversalContainerIndex);
        m_currentNode = INVALID_NODE;
    } else {
        m_shouldMarkLastNodeVisited = true;
    }

    return true;
}

void GenericTraversal::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PG;
(2) 	BEGIN
(3) 	U := N − {s}; V := {s}; W := ∅;
(4) 	FOR toți y ∈ N DO p(y) := 0;
(5) 	k := 1; o(s) := 1;
(6) 	FOR toți y ∈ U DO o(y) := ∞;
(7) 	WHILE V != ∅ DO
(8) 	BEGIN
(9) 	    se selectează un nod x din V;
(10)	    IF există arc(x, y) ∈ A și y ∈ U
(11)	       THEN U := U − {y}; V := V ∪ {y};
(12)	        p(y) := x; k := k + 1; o(y) := k
(13)	       ELSE V := V − {x}; W := W ∪ {x};
(14)	END;
(15) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void GenericTraversal::setStartNode(NodeIndex_t startNode) {
    m_traversalContainer.push_back(startNode);
    m_nodesInfo[startNode].m_visitOrder = ++m_currentVisitOrder;
}

void GenericTraversal::onFinishedAlgorithm() {
    auto& graphManager = m_graph->getGraphManager();
    graphManager.clearAlgorithmPath(ANALYZING_EDGE);
    graphManager.clearAlgorithmPath(VISITED_EDGE);

    ITimedAlgorithm::onFinishedAlgorithm();
}

void GenericTraversal::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList U, V, W, p, o;
    for (NodeIndex_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const auto state = getNodeState(nodeIndex);
        switch (state) {
            case NodeData::State::UNVISITED:
                U << QString::number(nodeIndex);
                break;
            case NodeData::State::VISITED:
            case NodeData::State::ANALYZING:
                V << QString::number(nodeIndex);
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

        if (m_nodesInfo[nodeIndex].m_visitOrder == 0) {
            o << "∞";
        } else {
            o << QString::number(m_nodesInfo[nodeIndex].m_visitOrder);
        }
    }

    infoLines << "U = {" + U.join(", ") + "}";
    infoLines << "V = {" + V.join(", ") + "}";
    infoLines << "W = {" + W.join(", ") + "}";
    infoLines << "";
    infoLines << "p: [" + p.join(", ") + "]";
    infoLines << "o: [" + o.join(", ") + "]";
    infoLines << "";
    infoLines << "k = " + QString::number(m_currentVisitOrder);

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void GenericTraversal::pickRandomNodeFromTraversalContainer() {
    m_traversalContainerIndex = Random::get().getSize(0, m_traversalContainer.size() - 1);
    m_currentNode = m_traversalContainer[m_traversalContainerIndex];
}

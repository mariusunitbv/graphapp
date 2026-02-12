#include <pch.h>

#include "PrimMST.h"

PrimMST::PrimMST(Graph* graph) : ITimedAlgorithm(graph) {
    m_nodeInfo.resize(m_graph->getGraphManager().getNodesCount());
    m_nodeInfo[0].setCost(0);

    graph->getGraphManager().setAlgorithmPathColor(MST_EDGE, qRgb(60, 179, 113));
}

bool PrimMST::step() {
    if (m_currentNode == INVALID_NODE) {
        pickLowestCostNode();
    }

    if (m_currentNode == INVALID_NODE) {
        return false;
    }

    if (!m_nodeInfo[m_currentNode].m_inMST) {
        setNodeState(m_currentNode, NodeData::State::ANALYZING);
        m_nodeInfo[m_currentNode].m_inMST = true;

        if (m_nodeInfo[m_currentNode].m_parent != INVALID_NODE) {
            m_pseudocodeForm.highlight({8, 9, 10, 11});
            m_graph->getGraphManager().addAlgorithmEdge(m_nodeInfo[m_currentNode].m_parent,
                                                        m_currentNode, MST_EDGE);
        } else {
            if (m_currentNode != 0) {
                QMessageBox::information(nullptr, "Prim MST",
                                         "The graph is disconnected. MST cannot be completed.",
                                         QMessageBox::Ok);

                return false;
            }

            m_pseudocodeForm.highlight({8, 9});
        }

        return true;
    }

    m_graph->getGraphManager().getGraphStorage()->forEachOutgoingEdgeWithOpposites(
        m_currentNode, [&](NodeIndex_t neighbour, CostType_t cost) {
            if (m_nodeInfo[neighbour].m_inMST) {
                return;
            }

            if (!m_nodeInfo[neighbour].m_minimalCostInitialized ||
                cost < m_nodeInfo[neighbour].m_minimalCost) {
                m_nodeInfo[neighbour].setCost(cost);
                m_nodeInfo[neighbour].m_parent = m_currentNode;
            }
        });

    setNodeState(m_currentNode, NodeData::State::VISITED);
    m_currentNode = INVALID_NODE;

    m_pseudocodeForm.highlight({12, 13, 14});

    return true;
}

void PrimMST::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PRIM;
(2) BEGIN
(3)     v(0) := 0; N1 := ∅; A0 := ∅; N1 := N - N1;
(4)     FOR i := 1 TO n DO
(5)         v(i) := ∞;
(6)     WHILE N1 ≠ N DO
(7)     BEGIN
(8)         v(y) := min{v(x) | x ∈ N1};
(9)         N1 := N1 ∪ {y}; N1 := N1 - {y};
(10)        IF y ≠ 0 THEN
(11)            A0 := A0 ∪ {e(y)};
(12)        FOR [y, y'] ∈ E(y) ∩ [N1, N1] DO
(13)            IF v(y') > b[y, y'] 
(14)              THEN BEGIN v(y') := b[y, y']; e(y') := [y, y']; END;
(15)     END;
(16) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

void PrimMST::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList v, N1, N1_, A;
    for (NodeIndex_t i = 0; i < nodeCount; ++i) {
        const auto& nodeInfo = m_nodeInfo[i];

        if (!nodeInfo.m_minimalCostInitialized) {
            v << "∞";
        } else {
            v << QString::number(nodeInfo.m_minimalCost);
        }

        if (nodeInfo.m_inMST) {
            N1 << QString::number(i);
        } else {
            N1_ << QString::number(i);
        }

        if (nodeInfo.m_inMST && nodeInfo.m_parent != INVALID_NODE) {
            A << QString("[%1, %2]").arg(nodeInfo.m_parent).arg(i);
        }
    }

    infoLines << "v: [" + v.join(", ") + "]";
    infoLines << "N1: {" + N1.join(", ") + "}";
    infoLines << "N1': {" + N1_.join(", ") + "}";
    infoLines << "A0: {" + A.join(", ") + "}";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void PrimMST::resetForUndo() {
    for (auto& info : m_nodeInfo) {
        info.m_parent = INVALID_NODE;
        info.m_minimalCost = std::numeric_limits<CostType_t>::max();
        info.m_inMST = false;
        info.m_minimalCostInitialized = false;
    }

    m_nodeInfo[0].setCost(0);

    m_currentNode = INVALID_NODE;
}

void PrimMST::pickLowestCostNode() {
    std::optional<CostType_t> minimalCost;
    for (NodeIndex_t i = 0; i < m_graph->getGraphManager().getNodesCount(); ++i) {
        if (m_nodeInfo[i].m_inMST) {
            continue;
        }

        if (!minimalCost || (minimalCost && m_nodeInfo[i].m_minimalCost < minimalCost.value())) {
            minimalCost = m_nodeInfo[i].m_minimalCost;
            m_currentNode = i;
        }
    }
}

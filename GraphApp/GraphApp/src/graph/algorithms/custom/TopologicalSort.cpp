#include <pch.h>

#include "TopologicalSort.h"

static constexpr auto TS_PSEUDOCODE_PRIORITY = 1;

TopologicalSort::TopologicalSort(Graph* graph) : DepthFirstTotalTraversal(graph) {
    graph->getGraphManager().setAlgorithmPathColor(BACK_EDGE, qRgb(220, 20, 20));

    connect(this, &IAlgorithm::pickedStartNode, this, &TopologicalSort::onPickedNewStartNode);
    connect(this, &IAlgorithm::visitedNode, this, &TopologicalSort::onVisitedNode);
    connect(this, &IAlgorithm::analyzingNode, this, &TopologicalSort::onAnalyzingNode);
    connect(this, &IAlgorithm::analyzedNode, this, &TopologicalSort::onAnalyzedNode);

    m_pseudocodeForm.setHighlightPriority(TS_PSEUDOCODE_PRIORITY);
}

bool TopologicalSort::step() {
    if (!m_backEdges.empty()) {
        return false;
    }

    return DepthFirstTotalTraversal::step();
}

void TopologicalSort::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1)  PROGRAM ST;
(2)  BEGIN;
(3)      U := N − {s}; V := {s}; W := ∅;
(4)      TO := [];
(5)      FOR toți y ∈ N DO p(y) := 0;
(6)      t := 1; t1(s) := 1; t2(s) := ∞;
(7)      FOR toți y ∈ U DO t1(y) := ∞; t2(y) := ∞;
(8)      WHILE W ≠ N DO
(9)      BEGIN
(10)         WHILE V ≠ ∅ DO
(11)         BEGIN
(12)             se selectează cel mai nou nod x introdus în V;
(13)             IF există arc (x, y) ∈ A și y ∈ U
(14)             THEN U := U − {y}; V := V ∪ {y}; p(y) := x;
(15)                  t := t + 1; t1(y) := t
(16)             ELSE V := V − {x}; W := W ∪ {x};
(17)                  t := t + 1; t2(x) := t;
(18)                  adaugă x la începutul listei TO;
(19)         END;
(20)         se selectează s ∈ U; U := U − {s}; V := {s};
(21)         t := t + 1; t1(s) := t;
(22)     END;
(23) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1}, TS_PSEUDOCODE_PRIORITY);
}

void TopologicalSort::onFinishedAlgorithm() {
    auto& graphManager = m_graph->getGraphManager();

    if (!m_backEdges.empty()) {
        const auto [start, end] = m_backEdges.back();
        graphManager.addAlgorithmEdge(start, end, BACK_EDGE);
        m_pseudocodeForm.highlight({23}, TS_PSEUDOCODE_PRIORITY);

        QMessageBox::warning(
            nullptr, "Topological Sort",
            "The graph contains at least one cycle. Topological sorting is not possible.");
    } else {
        graphManager.clearAlgorithmPath(VISITED_EDGE);
    }

    ITimedAlgorithm::onFinishedAlgorithm();
}

void TopologicalSort::updateAlgorithmInfoText() const {
    auto& graphManager = m_graph->getGraphManager();

    const auto nodeCount = graphManager.getNodesCount();
    if (nodeCount > 100) {
        graphManager.setAlgorithmInfoText("Too many nodes to show information");
        return;
    }

    QStringList infoLines;

    QStringList U, V, W, p, t1, t2, to, P, I, R, T;
    for (auto node : m_traversalContainer) {
        V << QString::number(node);
    }

    for (auto node : m_topologicalOrder) {
        to << QString::number(node);
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
    infoLines << "TO = {" + to.join(", ") + "}";
    infoLines << "";
    infoLines << "P = {" + P.join(", ") + "}";
    infoLines << "I = {" + I.join(", ") + "}";
    infoLines << "R = {" + R.join(", ") + "}";
    infoLines << "T = {" + T.join(", ") + "}";

    graphManager.setAlgorithmInfoText(infoLines.join("\n"));
}

void TopologicalSort::onPickedNewStartNode(NodeIndex_t startNode) {
    if (m_stepTimer.isActive()) {
        m_pseudocodeForm.highlight({20, 21}, TS_PSEUDOCODE_PRIORITY);
    }
}

void TopologicalSort::onVisitedNode(NodeIndex_t node) {
    m_pseudocodeForm.highlight({14, 15}, TS_PSEUDOCODE_PRIORITY);
}

void TopologicalSort::onAnalyzingNode(NodeIndex_t node) {
    m_pseudocodeForm.highlight({12}, TS_PSEUDOCODE_PRIORITY);
}

void TopologicalSort::onAnalyzedNode(NodeIndex_t node) {
    m_topologicalOrder.push_front(node);
    m_pseudocodeForm.highlight({16, 17, 18}, TS_PSEUDOCODE_PRIORITY);
}

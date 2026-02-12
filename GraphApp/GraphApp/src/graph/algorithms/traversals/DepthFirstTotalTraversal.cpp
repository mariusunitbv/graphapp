#include <pch.h>

#include "DepthFirstTotalTraversal.h"

#include "../random/Random.h"

DepthFirstTotalTraversal::DepthFirstTotalTraversal(Graph* graph) : DepthFirstTraversal(graph) {
    m_isTotalTraversal = true;

    m_randomOrderNodes.resize(m_graph->getGraphManager().getNodesCount());
    std::iota(m_randomOrderNodes.begin(), m_randomOrderNodes.end(), 0);
    std::shuffle(m_randomOrderNodes.begin(), m_randomOrderNodes.end(), Random::get().getEngine());
}

bool DepthFirstTotalTraversal::step() {
    if (m_traversalContainer.empty()) {
        return pickNewStartNode();
    }

    return DepthFirstTraversal::step();
}

void DepthFirstTotalTraversal::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1)  PROGRAM PTDF;
(2)  BEGIN;
(3)      U := N − {s}; V := {s}; W := ∅;
(4)      FOR toți y ∈ N DO p(y) := 0;
(5)      t := 1; t1(s) := 1; t2(s) := ∞;
(6)      FOR toți y ∈ U DO t1(y) := ∞; t2(y) := ∞;
(7)      WHILE W ≠ N DO
(8)      BEGIN
(9)          WHILE V ≠ ∅ DO
(10)         BEGIN
(11)             se selectează cel mai nou nod x introdus în V;
(12)             IF există arc (x, y) ∈ A și y ∈ U
(13)             THEN U := U − {y}; V := V ∪ {y}; p(y) := x;
(14)                  t := t + 1; t1(y) := t
(15)             ELSE V := V − {x}; W := W ∪ {x};
(16)                  t := t + 1; t2(x) := t;
(17)         END;
(18)         se selectează s ∈ U; U := U − {s}; V := {s};
(19)         t := t + 1; t1(s) := t;
(20)     END;
(21) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

bool DepthFirstTotalTraversal::pickNewStartNode() {
    for (; m_currentRandomIndex < m_randomOrderNodes.size(); ++m_currentRandomIndex) {
        const auto nodeIndex = m_randomOrderNodes[m_currentRandomIndex];
        if (getNodeState(nodeIndex) == NodeData::State::UNVISITED) {
            setStartNode(nodeIndex);
            setNodeState(nodeIndex, NodeData::State::VISITED);
            m_pseudocodeForm.highlight({18, 19});

            return true;
        }
    }

    return false;
}

void DepthFirstTotalTraversal::resetForUndo() {
    m_currentRandomIndex = 0;

    DepthFirstTraversal::resetForUndo();
}

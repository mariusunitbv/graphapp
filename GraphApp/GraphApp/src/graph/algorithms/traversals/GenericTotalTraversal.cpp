#include <pch.h>

#include "GenericTotalTraversal.h"

#include "../random/Random.h"

GenericTotalTraversal::GenericTotalTraversal(Graph* graph) : GenericTraversal(graph) {
    m_isTotalTraversal = true;

    m_randomOrderNodes.resize(m_graph->getGraphManager().getNodesCount());
    std::iota(m_randomOrderNodes.begin(), m_randomOrderNodes.end(), 0);
    std::shuffle(m_randomOrderNodes.begin(), m_randomOrderNodes.end(), Random::get().getEngine());
}

bool GenericTotalTraversal::step() {
    if (m_traversalContainer.empty()) {
        return pickNewStartNode();
    }

    return GenericTraversal::step();
}

void GenericTotalTraversal::showPseudocodeForm() {
    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PTG;
(2)   BEGIN
(3)       U := N − {s};  V := {s};  W := ∅;
(4)       FOR toți y ∈ N DO p(y) := 0;
(5)       k := 1;  o(s) := 1;
(6)       FOR toți y ∈ U DO o(y) := ∞;
(7)       WHILE W ≠ N DO
(8)       BEGIN
(9)           WHILE V ≠ ∅ DO
(10)          BEGIN
(11)              se selectează un nod x din V;
(12)              IF există arc(x, y) ∈ A și y ∈ U
(13)              THEN U := U − {y}; V := V ∪ {y}; p(y) := x;  
(14)                  k := k + 1; o(y) := k
(15)              ELSE V := V − {x}; W := W ∪ {x};
(16)          END;
(17)          se selectează s ∈ U; U := U − {s}; V := {s};
(18)          	k := k + 1; o(s) := k;
(19)      END;
(20) END.
)"));

    IAlgorithm::showPseudocodeForm();
    m_pseudocodeForm.highlight({1});
}

bool GenericTotalTraversal::pickNewStartNode() {
    for (; m_currentRandomIndex < m_randomOrderNodes.size(); ++m_currentRandomIndex) {
        const auto nodeIndex = m_randomOrderNodes[m_currentRandomIndex];
        if (getNodeState(nodeIndex) == NodeData::State::UNVISITED) {
            setStartNode(nodeIndex);
            setNodeState(nodeIndex, NodeData::State::VISITED);
            m_pseudocodeForm.highlight({17, 18});

            return true;
        }
    }

    return false;
}

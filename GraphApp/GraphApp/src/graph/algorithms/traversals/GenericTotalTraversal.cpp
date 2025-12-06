#include <pch.h>

#include "GenericTotalTraversal.h"

GenericTotalTraversal::GenericTotalTraversal(Graph* parent) : GenericTraversal(parent) {
    m_isTotalTraversal = true;
}

void GenericTotalTraversal::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

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
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({6});

    TimedInteractiveAlgorithm::showPseudocode();
}

bool GenericTotalTraversal::stepOnce() {
    if (m_currentNode == getAllNodes().size()) {
        return false;
    }

    if (m_currentNode == m_nodesVector.size()) {
        return pickAnotherNode();
    }

    return GenericTraversal::stepOnce();
}

void GenericTotalTraversal::stepAll() {
    while (m_currentNode != getAllNodes().size()) {
        GenericTraversal::stepAll();
        pickAnotherNode();
    }
}

bool GenericTotalTraversal::pickAnotherNode() {
    Node* unvisitedNode = getRandomUnvisitedNode();
    if (!unvisitedNode) {
        return false;
    }

    unvisitedNode->markVisited();

    m_discoveryOrder[unvisitedNode->getIndex()] = ++m_discoveryCount;

    m_pseudocodeForm.highlight({17, 18});
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_parentsLabel.compute();
    m_discoveryLabel.compute();

    m_nodesVector.push_back(unvisitedNode);
    return true;
}

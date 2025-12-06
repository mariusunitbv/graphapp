#include <pch.h>

#include "DepthTotalTraversal.h"

DepthTotalTraversal::DepthTotalTraversal(Graph* parent) : DepthTraversal(parent) {
    m_isTotalTraversal = true;
}

void DepthTotalTraversal::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

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
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({6});

    TimedInteractiveAlgorithm::showPseudocode();
}

bool DepthTotalTraversal::stepOnce() {
    if (m_nodesStack.empty()) {
        return pickAnotherNode();
    }

    return DepthTraversal::stepOnce();
}

void DepthTotalTraversal::stepAll() {
    while (!m_nodesStack.empty()) {
        DepthTraversal::stepAll();
        pickAnotherNode();
    }
}

bool DepthTotalTraversal::pickAnotherNode() {
    Node* unvisitedNode = getRandomUnvisitedNode();
    if (!unvisitedNode) {
        return false;
    }

    unvisitedNode->markVisited();

    m_nodesStack.push(unvisitedNode);
    m_discoveryTimes[unvisitedNode->getIndex()] = ++m_time;

    m_pseudocodeForm.highlight({18, 19});
    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_discoveryTimesLabel.compute();

    return true;
}

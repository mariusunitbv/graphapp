#include <pch.h>

#include "BreadthTraversal.h"

BreadthTraversal::BreadthTraversal(Graph* parent)
    : TraversalBase(parent), m_lengths(parent->getNodesCount(), -1), m_lengthsLabel(parent, 5) {
    m_visitedLabel.setComputeFunction([&](auto label) {
        auto queue = m_nodesQueue;

        QStringList list;
        while (!queue.empty()) {
            list << QString::number(queue.front()->getIndex());
            queue.pop();
        }

        label->setPlainText("V = { " + list.join(", ") + " }");
    });

    m_lengthsLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto length : m_lengths) {
            list << (length == -1 ? "∞" : QString::number(length));
        }

        label->setPlainText("l = [ " + list.join(", ") + " ]");
    });
}

void BreadthTraversal::setStartNode(Node* node) {
    m_nodesQueue.push(node);
    m_lengths[node->getIndex()] = 0;
}

void BreadthTraversal::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(
        R"((1) PROGRAM PBF;
(2)   BEGIN
(3)       U := N − {s};  V := {s};  W := ∅;
(4)       FOR toți y ∈ N DO p(y) := 0;
(5)       l(s) := 0;
(6)       FOR toți y ∈ U DO l(y) := ∞;
(7)       WHILE V ≠ ∅ DO
(8)       BEGIN
(9)           se selectează cel mai vechi nod x introdus în V;
(10)          FOR (x, y) ∈ A DO
(11)              IF y ∈ U
(12)              THEN U := U − {y};  V := V ∪ {y};  p(y) := x;
(13)                   l(y) := l(x) + 1;
(14)          V := V − {x};  W := W ∪ {x};
(15)      END;
(16) END.
)");
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlightLine(6);

    TimedInteractiveAlgorithm::showPseudocode();
}

bool BreadthTraversal::stepOnce() {
    if (m_nodesQueue.empty()) {
        return false;
    }

    Node* x = m_nodesQueue.front();
    if (x->getState() != Node::State::CURRENTLY_ANALYZED) {
        x->markCurrentlyAnalyzed();
        x->update();

        m_pseudocodeForm.highlightLine(9);
        m_parentsLabel.compute();
        m_unvisitedLabel.compute();
        m_visitedLabel.compute();
        m_lengthsLabel.compute();

        return true;
    }

    if (hasNeighbours(x)) {
        const auto& neighbours = getNeighboursOf(x);
        for (Node* y : neighbours) {
            if (y->getState() != Node::State::UNVISITED) {
                continue;
            }

            y->markVisited(x);

            m_nodesQueue.push(y);
            m_nodesParent[y->getIndex()] = x->getIndex();
            m_lengths[y->getIndex()] = m_lengths[x->getIndex()] + 1;

            m_pseudocodeForm.highlightLines({12, 13});
            m_parentsLabel.compute();
            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_lengthsLabel.compute();

            return true;
        }
    }

    x->markAnalyzed();
    m_nodesQueue.pop();

    m_pseudocodeForm.highlightLine(14);
    m_visitedLabel.compute();
    m_analyzedLabel.compute();

    return true;
}

void BreadthTraversal::stepAll() {
    while (!m_nodesQueue.empty()) {
        Node* x = m_nodesQueue.front();
        x->markCurrentlyAnalyzed();

        if (hasNeighbours(x)) {
            const auto& neighbours = getNeighboursOf(x);
            for (Node* y : neighbours) {
                if (y->getState() != Node::State::UNVISITED) {
                    continue;
                }

                y->markVisited(x);

                m_nodesQueue.push(y);
                m_nodesParent[y->getIndex()] = x->getIndex();
                m_lengths[y->getIndex()] = m_lengths[x->getIndex()] + 1;
            }
        }

        x->markAnalyzed();
        m_nodesQueue.pop();
    }

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_lengthsLabel.compute();
}

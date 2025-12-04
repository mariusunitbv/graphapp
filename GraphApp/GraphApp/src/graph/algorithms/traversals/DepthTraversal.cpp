#include <pch.h>

#include "DepthTraversal.h"

DepthTraversal::DepthTraversal(Graph* parent)
    : TraversalBase(parent),
      m_discoveryTimes(parent->getNodesCount(), -1),
      m_analyzeTimes(parent->getNodesCount(), -1),
      m_discoveryTimesLabel(parent, 5),
      m_analyzeTimesLabel(parent, 6) {
    m_visitedLabel.setComputeFunction([&](auto label) {
        auto stack = m_nodesStack;

        QStringList list;
        while (!stack.empty()) {
            list << QString::number(stack.top()->getIndex());
            stack.pop();
        }

        label->setPlainText("V = { " + list.join(", ") + " }");
    });

    m_discoveryTimesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto discoveryTime : m_discoveryTimes) {
            list << (discoveryTime == -1 ? "∞" : QString::number(discoveryTime));
        }

        label->setPlainText("t1 = [ " + list.join(", ") + " ]");
    });

    m_analyzeTimesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto analyzedTime : m_analyzeTimes) {
            list << (analyzedTime == -1 ? "∞" : QString::number(analyzedTime));
        }

        label->setPlainText("t2 = [ " + list.join(", ") + " ]");
    });
}

void DepthTraversal::setStartNode(Node* node) {
    m_nodesStack.push(node);
    m_discoveryTimes[node->getIndex()] = ++m_time;
}

void DepthTraversal::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(
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

)");
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlightLine(6);

    TimedInteractiveAlgorithm::showPseudocode();
}

bool DepthTraversal::stepOnce() {
    if (m_nodesStack.empty()) {
        return false;
    }

    Node* x = m_nodesStack.top();
    if (x->getState() != Node::State::CURRENTLY_ANALYZED) {
        if (m_nodeToUnmarkFromBeingAnalyzed) {
            m_nodeToUnmarkFromBeingAnalyzed->markVisitedButNotAnalyzedAnymore();
            m_nodeToUnmarkFromBeingAnalyzed->update();
            m_nodeToUnmarkFromBeingAnalyzed = nullptr;
        }

        x->markCurrentlyAnalyzed();
        x->update();

        m_pseudocodeForm.highlightLine(9);
        m_parentsLabel.compute();
        m_unvisitedLabel.compute();
        m_visitedLabel.compute();
        m_discoveryTimesLabel.compute();
        m_analyzeTimesLabel.compute();

        return true;
    }

    if (hasNeighbours(x)) {
        const auto& neighbours = getNeighboursOf(x);
        for (Node* y : neighbours) {
            if (y->getState() != Node::State::UNVISITED) {
                continue;
            }

            y->markVisited(x);

            m_nodesStack.push(y);
            m_nodesParent[y->getIndex()] = x->getIndex();
            m_discoveryTimes[y->getIndex()] = ++m_time;

            m_pseudocodeForm.highlightLines({11, 12});
            m_parentsLabel.compute();
            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_discoveryTimesLabel.compute();

            m_nodeToUnmarkFromBeingAnalyzed = x;
            return true;
        }
    }

    x->markAnalyzed();
    m_analyzeTimes[x->getIndex()] = ++m_time;

    m_nodesStack.pop();

    m_pseudocodeForm.highlightLines({13, 14});
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_analyzeTimesLabel.compute();

    return true;
}

void DepthTraversal::stepAll() {
    while (!m_nodesStack.empty()) {
        Node* x = m_nodesStack.top();
        x->markCurrentlyAnalyzed();

        bool hasVisitedAllNeighbours = true;
        if (hasNeighbours(x)) {
            const auto& neighbours = getNeighboursOf(x);
            for (Node* y : neighbours) {
                if (y->getState() != Node::State::UNVISITED) {
                    continue;
                }

                y->markVisited(x);

                m_nodesStack.push(y);
                m_nodesParent[y->getIndex()] = x->getIndex();
                m_discoveryTimes[y->getIndex()] = ++m_time;

                hasVisitedAllNeighbours = false;
                break;
            }
        }

        if (!hasVisitedAllNeighbours) {
            x->markVisitedButNotAnalyzedAnymore();
            continue;
        }

        x->markAnalyzed();
        m_analyzeTimes[x->getIndex()] = ++m_time;

        m_nodesStack.pop();
    }

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_discoveryTimesLabel.compute();
    m_analyzeTimesLabel.compute();
}

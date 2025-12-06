#include <pch.h>

#include "DepthTraversal.h"

DepthTraversal::DepthTraversal(Graph* parent)
    : TraversalBase(parent),
      m_discoveryTimes(parent->getNodesCount(), -1),
      m_analyzeTimes(parent->getNodesCount(), -1),
      m_discoveryTimesLabel(parent, 5),
      m_analyzeTimesLabel(parent, 6),
      m_treeEdgesLabel(parent, 7),
      m_forwardEdgesLabel(parent, 8),
      m_backEdgesLabel(parent, 9),
      m_crossEdgesLabel(parent, 10) {
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

    m_treeEdgesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto [x, y] : m_treeEdges) {
            list << QString("(%1, %2)").arg(x).arg(y);
        }

        label->setPlainText("P = [ " + list.join(", ") + " ]");
    });

    m_forwardEdgesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto [x, y] : m_forwardEdges) {
            list << QString("(%1, %2)").arg(x).arg(y);
        }

        label->setPlainText("I = [ " + list.join(", ") + " ]");
    });

    m_backEdgesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto [x, y] : m_backEdges) {
            list << QString("(%1, %2)").arg(x).arg(y);
        }

        label->setPlainText("R = [ " + list.join(", ") + " ]");
    });

    m_crossEdgesLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto [x, y] : m_crossEdges) {
            list << QString("(%1, %2)").arg(x).arg(y);
        }

        label->setPlainText("T = [ " + list.join(", ") + " ]");
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
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({6});

    TimedInteractiveAlgorithm::showPseudocode();
}

const std::vector<size_t>& DepthTraversal::getAnalyzeTimes() const { return m_analyzeTimes; }

bool DepthTraversal::stepOnce() {
    if (m_nodesStack.empty()) {
        return false;
    }

    Node* x = m_nodesStack.top();
    if (x->getState() != Node::State::CURRENTLY_ANALYZING) {
        if (m_nodeToUnmarkFromBeingAnalyzed) {
            m_nodeToUnmarkFromBeingAnalyzed->markVisitedButNotAnalyzedAnymore();
            m_nodeToUnmarkFromBeingAnalyzed = nullptr;
        }

        x->markCurrentlyAnalyzed();

        m_pseudocodeForm.highlight({m_isTotalTraversal ? 11 : 9});
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

            y->markVisited();

            m_nodesStack.push(y);
            m_nodesParent[y->getIndex()] = x->getIndex();
            m_discoveryTimes[y->getIndex()] = ++m_time;
            m_treeEdges.emplace_back(x->getIndex(), y->getIndex());

            if (m_isTotalTraversal) {
                m_pseudocodeForm.highlight({13, 14});
            } else {
                m_pseudocodeForm.highlight({11, 12});
            }

            m_parentsLabel.compute();
            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_discoveryTimesLabel.compute();
            m_treeEdgesLabel.compute();

            m_nodeToUnmarkFromBeingAnalyzed = x;
            return true;
        }
    }

    x->markAnalyzed();
    m_analyzeTimes[x->getIndex()] = ++m_time;
    updateEdgesClassificationOfNode(x);

    m_nodesStack.pop();
    if (m_isTotalTraversal) {
        m_pseudocodeForm.highlight({15, 16});
    } else {
        m_pseudocodeForm.highlight({13, 14});
    }

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

                y->markVisited();

                m_nodesStack.push(y);
                m_nodesParent[y->getIndex()] = x->getIndex();
                m_discoveryTimes[y->getIndex()] = ++m_time;
                m_treeEdges.emplace_back(x->getIndex(), y->getIndex());

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

    updateAllEdgesClassification();

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_discoveryTimesLabel.compute();
    m_analyzeTimesLabel.compute();
    m_treeEdgesLabel.compute();
}

void DepthTraversal::updateEdgesClassificationOfNode(Node* x) {
    if (!hasNeighbours(x)) {
        return;
    }

    const auto& neighbours = getNeighboursOf(x);
    for (Node* y : neighbours) {
        if (y->getState() == Node::State::UNVISITED) {
            continue;
        }

        if (x == y) {
            m_backEdges.emplace_back(x->getIndex(), y->getIndex());
            m_backEdgesLabel.compute();
            continue;
        }

        if (m_nodesParent[y->getIndex()] == x->getIndex()) {
            continue;
        }

        const auto t1X = m_discoveryTimes[x->getIndex()];
        const auto t2X = m_analyzeTimes[x->getIndex()];

        const auto t1Y = m_discoveryTimes[y->getIndex()];
        const auto t2Y = m_analyzeTimes[y->getIndex()];

        if (t1X < t1Y && t1Y < t2Y && t2Y < t2X) {
            m_forwardEdges.emplace_back(x->getIndex(), y->getIndex());
            m_forwardEdgesLabel.compute();
        } else if (t1Y < t1X && t1X < t2X && t2X < t2Y) {
            m_backEdges.emplace_back(x->getIndex(), y->getIndex());
            m_backEdgesLabel.compute();
        } else if (t1Y < t2Y && t2Y < t1X && t1X < t2X) {
            m_crossEdges.emplace_back(x->getIndex(), y->getIndex());
            m_crossEdgesLabel.compute();
        }
    }
}

void DepthTraversal::updateAllEdgesClassification() {
    for (Node* x : getAllNodes()) {
        if (!hasNeighbours(x)) {
            continue;
        }

        const auto& neighbours = getNeighboursOf(x);
        for (Node* y : neighbours) {
            if (x == y) {
                m_backEdges.emplace_back(x->getIndex(), y->getIndex());
                continue;
            }

            if (m_nodesParent[y->getIndex()] == x->getIndex()) {
                continue;
            }

            const auto t1X = m_discoveryTimes[x->getIndex()];
            const auto t2X = m_analyzeTimes[x->getIndex()];

            const auto t1Y = m_discoveryTimes[y->getIndex()];
            const auto t2Y = m_analyzeTimes[y->getIndex()];

            if (t1X < t1Y && t1Y < t2Y && t2Y < t2X) {
                m_forwardEdges.emplace_back(x->getIndex(), y->getIndex());
            } else if (t1Y < t1X && t1X < t2X && t2X < t2Y) {
                m_backEdges.emplace_back(x->getIndex(), y->getIndex());
            } else if (t1Y < t2Y && t2Y < t1X && t1X < t2X) {
                m_crossEdges.emplace_back(x->getIndex(), y->getIndex());
            }
        }
    }

    m_forwardEdgesLabel.compute();
    m_backEdgesLabel.compute();
    m_crossEdgesLabel.compute();
}

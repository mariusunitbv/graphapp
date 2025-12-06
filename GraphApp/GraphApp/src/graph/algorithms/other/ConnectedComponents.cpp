#include <pch.h>

#include "ConnectedComponents.h"

#include "../random/Random.h"

ConnectedComponents::ConnectedComponents(Graph* parent)
    : TraversalBase(parent),
      m_connectedComponentsCountLabel(parent, 5),
      m_connectedComponentsLabel(parent, 6) {
    m_parentsLabel.hide();

    m_visitedLabel.setComputeFunction([&](auto label) {
        auto stack = m_nodesStack;

        QStringList list;
        while (!stack.empty()) {
            list << QString::number(stack.top()->getIndex());
            stack.pop();
        }

        label->setPlainText("V = { " + list.join(", ") + " }");
    });

    m_connectedComponentsCountLabel.setComputeFunction(
        [&](auto label) { label->setPlainText(QString("p: %1").arg(m_connectedComponentsCount)); });

    m_connectedComponentsLabel.setComputeFunction([&](auto label) {
        QStringList list;
        for (auto node : m_connectedComponentNodes) {
            list << QString::number(node->getIndex());
        }

        label->setPlainText("N' = { " + list.join(", ") + " }");
    });

    setStartNode(m_graph->getRandomNode());
}

void ConnectedComponents::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"(( 1) PROGRAM CC;
( 2)  BEGIN
( 3)      U := N − {s}; V := {s}; W := ∅; p := 1; N′ = {s};
( 4)      WHILE W ≠ N DO
( 5)      BEGIN
( 6)          WHILE V ≠ ∅ DO
( 7)          BEGIN
( 8)              se selectează cel mai nou nod x introdus în V;
( 9)              IF există muchie [x, y] ∈ A și y ∈ U
(10)              THEN U := U − {y}; V := V ∪ {y}; N′ := N′ ∪ {y}
(11)              ELSE V := V − {x}; W := W ∪ {x};
(12)          END;
(13)          se tipăresc p și N′;
(14)          se selectează s ∈ U; U := U − {s}; V := {s}; p := p + 1;
(15)          N′ := {s};
(16)  END;
(17) END.
)"));
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({6});

    TimedInteractiveAlgorithm::showPseudocode();
}

void ConnectedComponents::setStartNode(Node* node) {
    m_nodesStack.push(node);
    m_connectedComponentNodes.push_back(node);
}

bool ConnectedComponents::stepOnce() {
    if (m_nodesStack.empty()) {
        if (!m_connectedComponentNodes.empty()) {
            const auto randomColor = Random::get().getColor();
            for (auto node : m_connectedComponentNodes) {
                node->markPartOfConnectedComponent(randomColor);
            }

            m_connectedComponentsCount++;
            m_connectedComponentNodes.clear();

            m_pseudocodeForm.highlight({13});
            m_connectedComponentsCountLabel.compute();

            return true;
        }

        Node* unvisitedNode = getRandomUnvisitedNode();
        if (unvisitedNode) {
            setStartNode(unvisitedNode);
            unvisitedNode->markVisited();

            m_pseudocodeForm.highlight({14, 15});
            m_connectedComponentsLabel.compute();

            return true;
        }

        return false;
    }

    Node* x = m_nodesStack.top();
    if (x->getState() != Node::State::CURRENTLY_ANALYZING) {
        if (m_nodeToUnmarkFromBeingAnalyzed) {
            m_nodeToUnmarkFromBeingAnalyzed->markVisitedButNotAnalyzedAnymore();
            m_nodeToUnmarkFromBeingAnalyzed = nullptr;
        }

        x->markCurrentlyAnalyzed();

        m_pseudocodeForm.highlight({8});

        m_unvisitedLabel.compute();
        m_visitedLabel.compute();
        m_connectedComponentsCountLabel.compute();
        m_connectedComponentsLabel.compute();

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
            m_connectedComponentNodes.push_back(y);

            m_pseudocodeForm.highlight({9, 10});
            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_connectedComponentsCountLabel.compute();
            m_connectedComponentsLabel.compute();

            m_nodeToUnmarkFromBeingAnalyzed = x;
            return true;
        }
    }

    x->markAnalyzed();

    m_nodesStack.pop();

    m_pseudocodeForm.highlight({11});
    m_visitedLabel.compute();
    m_analyzedLabel.compute();

    return true;
}

void ConnectedComponents::stepAll() {
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
                m_connectedComponentNodes.push_back(y);

                hasVisitedAllNeighbours = false;
                break;
            }
        }

        if (!hasVisitedAllNeighbours) {
            x->markVisitedButNotAnalyzedAnymore();
            continue;
        }

        x->markAnalyzed();

        m_nodesStack.pop();
        if (m_nodesStack.empty()) {
            if (!m_connectedComponentNodes.empty()) {
                const auto randomColor = Random::get().getColor();
                for (auto node : m_connectedComponentNodes) {
                    node->markPartOfConnectedComponent(randomColor);
                }

                m_connectedComponentsCount++;
                m_connectedComponentNodes.clear();
            }

            Node* unvisitedNode = getRandomUnvisitedNode();
            if (unvisitedNode) {
                setStartNode(unvisitedNode);
            }
        }
    }

    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_connectedComponentsCountLabel.compute();
}

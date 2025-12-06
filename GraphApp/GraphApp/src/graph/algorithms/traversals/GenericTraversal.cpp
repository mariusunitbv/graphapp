#include <pch.h>

#include "GenericTraversal.h"

#include "../random/Random.h"

GenericTraversal::GenericTraversal(Graph* parent)
    : TraversalBase(parent),
      m_discoveryOrder(parent->getNodesCount(), -1),
      m_discoveryLabel(parent, 5) {
    m_nodesVector.reserve(parent->getNodesCount());

    m_visitedLabel.setComputeFunction([this](auto label) {
        QStringList list;
        const auto& nodes = m_graph->getNodes();
        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto state = nodes[i]->getState();
            if (state == Node::State::VISITED || state == Node::State::CURRENTLY_ANALYZING) {
                list << QString::number(i);
            }
        }

        label->setPlainText("V = { " + list.join(", ") + " }");
    });

    m_discoveryLabel.setComputeFunction([this](auto label) {
        QStringList list;
        for (auto order : m_discoveryOrder) {
            list << (order == -1 ? "∞" : QString::number(order));
        }

        label->setPlainText("o = [ " + list.join(", ") + " ]");
    });
}

void GenericTraversal::setStartNode(Node* node) {
    m_nodesVector.push_back(node);
    m_discoveryOrder[node->getIndex()] = ++m_discoveryCount;
}

void GenericTraversal::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM PG;
(2) 	BEGIN
(3) 	U := N − {s}; V := {s}; W := ∅;
(4) 	FOR toți y ∈ N DO p(y) := 0;
(5) 	k := 1; o(s) := 1;
(6) 	FOR toți y ∈ U DO o(y) := ∞;
(7) 	WHILE V != ∅ DO
(8) 	BEGIN
(9) 	    se selectează un nod x din V;
(10)	    IF există arc(x, y) ∈ A și y ∈ U
(11)	       THEN U := U − {y}; V := V ∪ {y};
(12)	        p(y) := x; k := k + 1; o(y) := k
(13)	       ELSE V := V − {x}; W := W ∪ {x};
(14)	END;
(15) END.
)"));
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({6});

    TimedInteractiveAlgorithm::showPseudocode();
}

bool GenericTraversal::stepOnce() {
    if (m_currentNode == m_nodesVector.size()) {
        return false;
    }

    Node* x = m_nodesVector[m_currentNode];
    if (x->getState() != Node::State::CURRENTLY_ANALYZING) {
        x->markCurrentlyAnalyzed();

        m_pseudocodeForm.highlight({m_isTotalTraversal ? 11 : 9});
        m_parentsLabel.compute();
        m_unvisitedLabel.compute();
        m_visitedLabel.compute();
        m_discoveryLabel.compute();

        return true;
    }

    if (hasNeighbours(x)) {
        const auto& neighbours = getNeighboursOf(x);
        for (Node* y : neighbours) {
            if (y->getState() != Node::State::UNVISITED) {
                continue;
            }

            y->markVisited();
            m_nodesParent[y->getIndex()] = x->getIndex();
            m_discoveryOrder[y->getIndex()] = ++m_discoveryCount;

            if (m_isTotalTraversal) {
                m_pseudocodeForm.highlight({13, 14});
            } else {
                m_pseudocodeForm.highlight({11, 12});
            }

            m_parentsLabel.compute();
            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_discoveryLabel.compute();

            m_nodesVector.insert(m_nodesVector.begin() +
                                     Random::get().getSize(m_currentNode + 1, m_nodesVector.size()),
                                 y);

            return true;
        }
    }

    x->markAnalyzed();

    ++m_currentNode;

    m_pseudocodeForm.highlight({m_isTotalTraversal ? 15 : 13});
    m_visitedLabel.compute();
    m_analyzedLabel.compute();

    return true;
}

void GenericTraversal::stepAll() {
    while (m_currentNode < m_nodesVector.size()) {
        Node* x = m_nodesVector[m_currentNode];
        x->markCurrentlyAnalyzed();

        if (hasNeighbours(x)) {
            const auto& neighbours = getNeighboursOf(x);
            for (Node* y : neighbours) {
                if (y->getState() != Node::State::UNVISITED) {
                    continue;
                }

                y->markVisited();
                m_nodesParent[y->getIndex()] = x->getIndex();
                m_discoveryOrder[y->getIndex()] = ++m_discoveryCount;

                m_nodesVector.insert(
                    m_nodesVector.begin() +
                        Random::get().getSize(m_currentNode + 1, m_nodesVector.size()),
                    y);
            }
        }

        x->markAnalyzed();
        ++m_currentNode;
    }

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_discoveryLabel.compute();
}

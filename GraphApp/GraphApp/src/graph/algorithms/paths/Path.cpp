#include <pch.h>

#include "Path.h"

Path::Path(Graph* parent)
    : TimedInteractiveAlgorithm(parent), m_genericTraversal(new GenericTraversal(m_graph)) {}

Path::~Path() { m_genericTraversal->deleteLater(); }

void Path::start(Node* start) {
    m_genericTraversal->setStartNode(start);
    m_genericTraversal->start();

    unmarkAllButUnvisitedNodes();

    Node* sourceNode = m_graph->getNodes()[m_sourceNode];
    if (sourceNode->getState() == Node::State::UNVISITED) {
        sourceNode->markUnreachable();
        sourceNode->update();
        m_pseudocodeForm.highlightLine(10);

        QMessageBox::information(nullptr, "Path",
                                 QString("Node %1 is unreachable from Node %2.")
                                     .arg(m_sourceNode)
                                     .arg(start->getIndex()));
        emit finished();
    }

    m_currentNode = sourceNode;

    if (m_stepDelay == 0) {
        stepAll();
        emit finished();
    } else {
        m_stepTimer.start(m_stepDelay);
    }
}

void Path::setSourceNodeIndex(size_t sourceNode) { m_sourceNode = sourceNode; }

void Path::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(
        R"((1) PROCEDURA DRUM(G, s, y);
(2)   BEGIN
(3)       se tipărește y;
(4)       WHILE p(y) ≠ 0 DO
(5)       BEGIN
(6)           x := p(y);
(7)           se tipărește x;
(8)           y := x
(9)       END;
(10) END.
)");
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlightLine(2);

    TimedInteractiveAlgorithm::showPseudocode();
}

bool Path::stepOnce() {
    Node* parent = m_genericTraversal->getParent(m_currentNode);

    if (m_currentNode->getState() != Node::State::PATH) {
        m_currentNode->markPath(parent);
        m_currentNode->update();

        m_pseudocodeForm.highlightLine(3);
        return true;
    }

    m_currentNode = parent;
    if (parent) {
        m_pseudocodeForm.highlightLines({6, 7, 8});
    } else {
        m_pseudocodeForm.highlightLine(9);
    }

    return parent != nullptr;
}

void Path::stepAll() {
    while (m_currentNode) {
        Node* parent = m_genericTraversal->getParent(m_currentNode);

        m_currentNode->markPath(parent);
        m_currentNode->update();

        m_currentNode = parent;

        parent = m_genericTraversal->getParent(m_currentNode);
    }
}

void Path::unmarkAllButUnvisitedNodes() {
    for (Node* node : m_graph->getNodes()) {
        if (node->getState() != Node::State::UNVISITED) {
            node->markAvailableInPathFinding();
        }
    }
}

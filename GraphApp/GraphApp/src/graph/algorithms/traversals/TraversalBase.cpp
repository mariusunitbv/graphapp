#include <pch.h>

#include "TraversalBase.h"

#include "../random/Random.h"

TraversalBase::TraversalBase(Graph* parent)
    : TimedInteractiveAlgorithm(parent),
      m_nodesParent(parent->getNodesCount(), -1),
      m_unvisitedLabel(parent, 1),
      m_visitedLabel(parent, 2),
      m_analyzedLabel(parent, 3, 50),
      m_parentsLabel(parent, 4) {
    m_unvisitedLabel.setComputeFunction([this](auto label) {
        QStringList list;
        const auto& nodes = m_graph->getNodes();
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i]->getState() == Node::State::UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setPlainText("U = { " + list.join(", ") + " }");
    });

    m_analyzedLabel.setComputeFunction([this](auto label) {
        QStringList list;
        const auto& nodes = m_graph->getNodes();
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i]->getState() == Node::State::ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setPlainText("W = { " + list.join(", ") + " }");
    });

    m_parentsLabel.setComputeFunction([this](auto label) {
        QStringList list;
        for (auto parent : m_nodesParent) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setPlainText("p = [ " + list.join(", ") + " ]");
    });
}

TraversalBase::~TraversalBase() {
    unmarkNodes();
    m_graph->scene()->update();
}

void TraversalBase::start() {
    markAllNodesUnvisited();

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();

    if (m_stepDelay == 0) {
        stepAll();
        emit finished();
    } else {
        m_stepTimer.start(m_stepDelay);
    }
}

Node* TraversalBase::getParent(Node* node) const {
    if (!node) {
        return nullptr;
    }

    const auto parentIndex = m_nodesParent[node->getIndex()];
    if (parentIndex == -1) {
        return nullptr;
    }

    return getAllNodes()[parentIndex];
}

const std::vector<Node*>& TraversalBase::getAllNodes() const { return m_graph->getNodes(); }

bool TraversalBase::hasNeighbours(Node* node) const {
    return m_graph->getAdjacencyList().contains(node);
}

const std::unordered_set<Node*>& TraversalBase::getNeighboursOf(Node* node) const {
    return m_graph->getAdjacencyList().at(node);
}

Node* TraversalBase::getRandomUnvisitedNode() const {
    std::vector<Node*> unvisited;

    for (Node* node : getAllNodes()) {
        if (node->getState() == Node::State::UNVISITED) {
            unvisited.push_back(node);
        }
    }

    if (unvisited.empty()) {
        return nullptr;
    }

    std::shuffle(unvisited.begin(), unvisited.end(), Random::get().getEngine());
    return unvisited.front();
}

void TraversalBase::markAllNodesUnvisited() {
    for (Node* node : getAllNodes()) {
        node->markUnvisited();
    }
}

void TraversalBase::unmarkNodes() {
    for (Node* node : getAllNodes()) {
        node->unmark();
    }
}

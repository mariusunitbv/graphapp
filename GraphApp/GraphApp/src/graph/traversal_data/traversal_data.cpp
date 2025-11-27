#include <pch.h>

#include "traversal_data.h"

#include "../node/Node.h"

TraversalData::TraversalData(const std::vector<Node*>& nodes, QGraphicsView* view)
    : m_parents(nodes.size(), -1),
      m_visited(nodes.size(), UNVISITED),
      m_view(view),
      m_allNodes(&nodes) {
    m_parentsLabel = AlgorithmLabel(view->scene(), view->mapToScene(10, 100), [this](auto label) {
        QStringList list;
        for (auto parent : m_parents) {
            list << (parent == -1 ? "-" : QString::number(parent));
        }

        label->setText("p = [ " + list.join(", ") + " ]");
    });
}

GenericTraversal::GenericTraversal(const std::vector<Node*>& nodes, QGraphicsView* view)
    : TraversalData(nodes, view), m_orders(nodes.size(), -1) {
    m_unvisitedLabel = AlgorithmLabel(view->scene(), view->mapToScene(10, 10), [this](auto label) {
        QStringList list;
        for (size_t i = 0; i < m_allNodes->size(); ++i) {
            if (m_visited[i] == UNVISITED) {
                list << QString::number(i);
            }
        }

        label->setText("U = { " + list.join(", ") + " }");
    });

    m_visitedLabel = AlgorithmLabel(view->scene(), view->mapToScene(10, 30), [this](auto label) {
        QStringList list;
        for (size_t i = 0; i < m_allNodes->size(); ++i) {
            if (m_visited[i] == VISITED) {
                list << QString::number(i);
            }
        }

        label->setText("V = { " + list.join(", ") + " }");
    });

    m_analyzedLabel = AlgorithmLabel(view->scene(), view->mapToScene(10, 50), [this](auto label) {
        QStringList list;
        for (size_t i = 0; i < m_allNodes->size(); ++i) {
            if (m_visited[i] == ANALYZED) {
                list << QString::number(i);
            }
        }

        label->setText("W = { " + list.join(", ") + " }");
    });

    m_ordersLabel = AlgorithmLabel(view->scene(), view->mapToScene(10, 120), [this](auto label) {
        QStringList list;
        for (auto order : m_orders) {
            list << (order == -1 ? "-" : QString::number(order));
        }

        label->setText("o = [ " + list.join(", ") + " ]");
    });
}

bool GenericTraversal::finished() { return m_currentIndex >= m_nodesVector.size(); }

TotalGenericTraversal::TotalGenericTraversal(const std::vector<Node*>& nodes, QGraphicsView* view)
    : GenericTraversal(nodes, view) {}

bool TotalGenericTraversal::finished() {
    if (m_currentIndex >= m_nodesVector.size() && m_nodesVector.size() <= m_visited.size()) {
        pickAnotherNode();
    }

    if (m_nodesVector.size() == m_visited.size() && m_currentIndex >= m_nodesVector.size()) {
        return true;
    }

    return false;
}

void TotalGenericTraversal::pickAnotherNode() {
    for (auto node : *m_allNodes) {
        if (m_visited[node->getIndex()] == UNVISITED) {
            m_visited[node->getIndex()] = VISITED;
            m_orders[node->getIndex()] = ++m_order;

            m_unvisitedLabel.compute();
            m_visitedLabel.compute();
            m_ordersLabel.compute();

            m_nodesVector.push_back(node);

            break;
        }
    }
}

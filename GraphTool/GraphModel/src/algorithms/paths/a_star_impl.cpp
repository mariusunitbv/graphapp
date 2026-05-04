module;
#include <pch.h>

module a_star;

AlgorithmType AStar::getType() const { return AlgorithmType::A_STAR; }

const char* AStar::getName() const { return "A-star"; }

int AStar::calculateHeuristic(NodeIndex_t node) const {
    return static_cast<int>(m_model->heuristicDistance(node, m_targetNode));
}

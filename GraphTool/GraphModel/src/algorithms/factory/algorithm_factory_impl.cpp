module;
#include <pch.h>

module algorithm_factory;

import breadth_first_search;
import depth_first_search;

import dijkstra;
import a_star;
import a_star_landmark;

std::unique_ptr<IAlgorithm> AlgorithmFactory::createAlgorithm(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::BREADTH_FIRST_SEARCH:
            return std::make_unique<BreadthFirstSearch>();
        case AlgorithmType::DEPTH_FIRST_SEARCH:
            return std::make_unique<DepthFirstSearch>();
        case AlgorithmType::DIJKSTRA:
            return std::make_unique<Dijkstra>();
        case AlgorithmType::A_STAR:
            return std::make_unique<AStar>();
        case AlgorithmType::A_STAR_LANDMARK:
            return std::make_unique<AStarLandmark>();
        default:
            GAPP_THROW("Unsupported algorithm type");
    }

    return {};
}

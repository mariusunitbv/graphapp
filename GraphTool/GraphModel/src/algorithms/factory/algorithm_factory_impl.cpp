module;
#include <pch.h>

module algorithm_factory;

import breadth_first_search;
import depth_first_search;

import dijkstra;

std::unique_ptr<IAlgorithm> AlgorithmFactory::createAlgorithm(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::BREADTH_FIRST_SEARCH:
            return std::make_unique<BreadthFirstSearch>();
        case AlgorithmType::DEPTH_FIRST_SEARCH:
            return std::make_unique<DepthFirstSearch>();
        case AlgorithmType::DIJKSTRA:
            return std::make_unique<Dijkstra>();
        default:
            GAPP_THROW("Unsupported algorithm type");
    }

    return {};
}

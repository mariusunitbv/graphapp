#include <pch.h>

#include "FordFulkerson.h"

FordFulkerson::FordFulkerson(Graph* graph) : ITimedAlgorithm(graph) {}

void FordFulkerson::start(NodeIndex_t start, NodeIndex_t end) { ITimedAlgorithm::start(); }

bool FordFulkerson::step() { return false; }

void FordFulkerson::showPseudocodeForm() {}

void FordFulkerson::updateAlgorithmInfoText() const {}

module;
#include <pch.h>

module a_star_landmark;

AlgorithmType AStarLandmark::getType() const { return AlgorithmType::A_STAR_LANDMARK; }

const char* AStarLandmark::getName() const { return "A-Star Landmark"; }

void AStarLandmark::initialize() {
    Dijkstra::initialize();

    std::unordered_set<NodeIndex_t> usedLandmarks;
    for (int i = 0; i < 16; ++i) {
        const auto nodeIndex = xorshift32() % m_model->getNodeCount();
        if (usedLandmarks.contains(nodeIndex)) {
            continue;
        }

        addToATL(nodeIndex);
        usedLandmarks.insert(nodeIndex);
    }
}

int AStarLandmark::calculateHeuristic(NodeIndex_t node) const {
    int h = 0;

    for (const auto& atl : m_atl) {
        const auto a = atl[node];
        const auto b = atl[m_targetNode];

        if (a == INT_MAX || b == INT_MAX) {
            continue;
        }

        h = std::max(h, std::abs(a - b));
    }

    return h;
}

void AStarLandmark::addToATL(NodeIndex_t node) {
    std::vector<int> dist(m_model->getNodeCount(), INT_MAX);
    std::priority_queue<std::pair<int, NodeIndex_t>, std::vector<std::pair<int, NodeIndex_t>>,
                        std::greater<>>
        pq;

    dist[node] = 0;
    pq.push({0, node});

    while (!pq.empty()) {
        auto [cost, node] = pq.top();
        pq.pop();

        if (cost > dist[node]) continue;

        for (auto& [neighbour, edgeCost] : m_model->getNodeEdges(node)) {
            int newCost = dist[node] + edgeCost;
            if (newCost < dist[neighbour]) {
                dist[neighbour] = newCost;
                pq.push({newCost, neighbour});
            }
        }
    }

    m_atl.emplace_back(std::move(dist));
}

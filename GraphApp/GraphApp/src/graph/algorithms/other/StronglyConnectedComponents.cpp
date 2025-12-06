#include <pch.h>

#include "StronglyConnectedComponents.h"

#include "../form/intermediate_graph/IntermediateGraph.h"

#include "../random/Random.h"

StronglyConnectedComponents::StronglyConnectedComponents(Graph* parent)
    : Algorithm(parent), m_traversal(new DepthTotalTraversal(parent)) {
    connect(m_traversal, &TraversalBase::finished, this,
            &StronglyConnectedComponents::onTraversalFinished);
}

void StronglyConnectedComponents::showPseudocode() {
    if (m_stepDelay == 0) {
        return;
    }

    m_pseudocodeForm.setPseudocodeText(QStringLiteral(
        R"((1) PROGRAM CTC;
(2) BEGIN
(3)     PROCEDURA PTDF(G);
(4)     PROCEDURA INVERSARE(G);
(5)     PROCEDURA PTDF(G⁻¹);
(6)     PROCEDURA SEPARARE(G⁻¹);
(7) END.
)"));
    m_pseudocodeForm.show();
    m_pseudocodeForm.highlight({2});

    Algorithm::showPseudocode();
}

void StronglyConnectedComponents::start() {
    m_pseudocodeForm.highlight({3});

    m_traversal->setStartNode(m_graph->getRandomNode());
    m_traversal->setStepDelay(m_stepDelay);
    m_traversal->start();
}

void StronglyConnectedComponents::setStepDelay(int stepDelay) { m_stepDelay = stepDelay; }

void StronglyConnectedComponents::onTraversalFinished() {
    m_invertedGraph = m_graph->getInvertedGraph();
    if (!m_invertedGraph) {
        return;
    }

    const auto window = new IntermediateGraph(m_invertedGraph);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();

    m_pseudocodeForm.highlight({4});
    QTimer::singleShot(m_stepDelay, this, &StronglyConnectedComponents::beginInvertedTraversal);
}

void StronglyConnectedComponents::beginInvertedTraversal() {
    m_pseudocodeForm.highlight({5});
    m_traversalOfInverted = new CustomDepthTotalTraversal(m_invertedGraph, m_traversal);
    connect(m_traversalOfInverted, &TraversalBase::finished, this,
            &StronglyConnectedComponents::onInvertedTraversalFinished);

    m_traversalOfInverted->setStepDelay(m_stepDelay);
    m_traversalOfInverted->start();
}

void StronglyConnectedComponents::onInvertedTraversalFinished() {
    m_pseudocodeForm.highlight({6});
    QTimer::singleShot(m_stepDelay, this, [this]() {
        const auto window = new IntermediateGraph(m_traversalOfInverted->getMergedGraph());
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();

        emit finished();
    });
}

StronglyConnectedComponents::CustomDepthTotalTraversal::CustomDepthTotalTraversal(
    Graph* parent, DepthTraversal* rhs)
    : DepthTraversal(parent) {
    const auto& analyzeTimes = rhs->getAnalyzeTimes();
    m_sortedAnalyzeFinishOrder.reserve(analyzeTimes.size());

    for (size_t nodeIndex = 0; nodeIndex < analyzeTimes.size(); ++nodeIndex) {
        m_sortedAnalyzeFinishOrder.emplace_back(nodeIndex, analyzeTimes[nodeIndex]);
    }

    std::sort(m_sortedAnalyzeFinishOrder.begin(), m_sortedAnalyzeFinishOrder.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    const auto& nodes = getAllNodes();
    setStartNode(nodes[m_sortedAnalyzeFinishOrder[0].first]);
}

Node* StronglyConnectedComponents::CustomDepthTotalTraversal::getNextNodeToAnalyze() {
    for (const auto& [nodeIndex, _] : m_sortedAnalyzeFinishOrder) {
        Node* node = getAllNodes()[nodeIndex];
        if (node->getState() == Node::State::UNVISITED) {
            return node;
        }
    }

    return nullptr;
}

bool StronglyConnectedComponents::CustomDepthTotalTraversal::stepOnce() {
    if (m_nodesStack.empty()) {
        if (!m_currentComponentNodes.empty()) {
            const auto randomColor = Random::get().getColor();
            for (auto node : m_currentComponentNodes) {
                node->markPartOfConnectedComponent(randomColor);
            }

            std::sort(m_currentComponentNodes.begin(), m_currentComponentNodes.end(),
                      [](Node* a, Node* b) { return a->getIndex() < b->getIndex(); });

            m_latestStackSize = 0;
            m_stronglyConnectedComponents.push_back(std::move(m_currentComponentNodes));

            return true;
        }

        Node* unvisitedNode = getNextNodeToAnalyze();
        if (unvisitedNode) {
            setStartNode(unvisitedNode);
            unvisitedNode->markVisited();

            return true;
        }

        return false;
    }

    if (m_nodesStack.size() > m_latestStackSize) {
        m_latestStackSize = m_nodesStack.size();
        m_currentComponentNodes.push_back(m_nodesStack.top());
    }

    return DepthTraversal::stepOnce();
}

void StronglyConnectedComponents::CustomDepthTotalTraversal::stepAll() {
    m_currentComponentNodes.push_back(m_nodesStack.top());

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
                m_currentComponentNodes.push_back(y);

                m_nodesStack.push(y);
                m_nodesParent[y->getIndex()] = x->getIndex();
                m_discoveryTimes[y->getIndex()] = ++m_time;
                m_treeEdges.emplace_back(x->getIndex(), y->getIndex());

                hasVisitedAllNeighbours = false;
                break;
            }
        }

        if (!hasVisitedAllNeighbours) {
            x->markVisitedButNotAnalyzedAnymore();
            continue;
        }

        x->markAnalyzed();
        m_analyzeTimes[x->getIndex()] = ++m_time;

        m_nodesStack.pop();

        if (m_nodesStack.empty()) {
            if (!m_currentComponentNodes.empty()) {
                const auto randomColor = Random::get().getColor();
                for (auto node : m_currentComponentNodes) {
                    node->markPartOfConnectedComponent(randomColor);
                }

                std::sort(m_currentComponentNodes.begin(), m_currentComponentNodes.end(),
                          [](Node* a, Node* b) { return a->getIndex() < b->getIndex(); });

                m_stronglyConnectedComponents.push_back(std::move(m_currentComponentNodes));
            }

            Node* unvisitedNode = getNextNodeToAnalyze();
            if (unvisitedNode) {
                setStartNode(unvisitedNode);
                unvisitedNode->markVisited();

                m_currentComponentNodes.push_back(unvisitedNode);
            }
        }
    }

    updateAllEdgesClassification();

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_analyzedLabel.compute();
    m_discoveryTimesLabel.compute();
    m_analyzeTimesLabel.compute();
    m_treeEdgesLabel.compute();
}

Graph* StronglyConnectedComponents::CustomDepthTotalTraversal::getMergedGraph() const {
    const auto mergedGraph = new Graph();
    mergedGraph->setAnimationsDisabled(m_graph->getAnimationsDisabled());
    mergedGraph->setAllowLoops(m_graph->getAllowLoops());
    mergedGraph->setOrientedGraph(m_graph->getOrientedGraph());

    std::vector<size_t> oldToMergedNode(getAllNodes().size());
    std::unordered_map<Node*, std::unordered_set<Node*>> addedEdges;

    for (size_t i = 0; i < m_stronglyConnectedComponents.size(); ++i) {
        const auto& components = m_stronglyConnectedComponents[i];

        QPointF meanPos{};
        QStringList name;
        for (auto node : components) {
            meanPos += (node->pos() / components.size());
            name << node->getLabel();

            oldToMergedNode[node->getIndex()] = i;
        }

        mergedGraph->addNode(meanPos);

        auto lastNode = mergedGraph->getNodes().back();
        lastNode->setLabel(name.join(", "));
        lastNode->setFillColor(components[0]->getFillColor());
    }

    const auto& edges = m_graph->getEdges();
    const auto& mergedNodes = mergedGraph->getNodes();
    for (auto edge : edges) {
        Node* u = mergedNodes[oldToMergedNode[edge->getStartNode()->getIndex()]];
        Node* v = mergedNodes[oldToMergedNode[edge->getEndNode()->getIndex()]];
        if (u == v || addedEdges[u].contains(v)) {
            continue;
        }

        mergedGraph->addEdge(v, u, edge->getCost());
    }

    return mergedGraph;
}

bool StronglyConnectedComponents::CustomDepthTotalTraversal::pickAnotherNode() {
    Node* unvisitedNode = getNextNodeToAnalyze();
    if (!unvisitedNode) {
        return false;
    }

    unvisitedNode->markVisited();

    m_nodesStack.push(unvisitedNode);
    m_discoveryTimes[unvisitedNode->getIndex()] = ++m_time;

    m_parentsLabel.compute();
    m_unvisitedLabel.compute();
    m_visitedLabel.compute();
    m_discoveryTimesLabel.compute();

    return true;
}

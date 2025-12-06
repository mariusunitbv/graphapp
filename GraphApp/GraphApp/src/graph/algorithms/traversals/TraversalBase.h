#pragma once

#include "../TimedInteractiveAlgorithm.h"

#include "../../algorithm_text/AlgorithmText.h"

class TraversalBase : public TimedInteractiveAlgorithm {
    Q_OBJECT

   public:
    TraversalBase(Graph* parent);

    virtual ~TraversalBase();
    virtual void setStartNode(Node* node) = 0;

    void start();

    Node* getParent(Node* node) const;

   protected:
    const std::vector<Node*>& getAllNodes() const;

    bool hasNeighbours(Node* node) const;
    const std::unordered_set<Node*>& getNeighboursOf(Node* node) const;

    Node* getRandomUnvisitedNode() const;

    std::vector<size_t> m_nodesParent;
    AlgorithmText m_parentsLabel;
    AlgorithmText m_unvisitedLabel;
    AlgorithmText m_visitedLabel;
    AlgorithmText m_analyzedLabel;

   private:
    void markAllNodesUnvisited();
    void unmarkNodes();
};

#pragma once

#include "../TimedInteractiveAlgorithm.h"
#include "../traversals/GenericTraversal.h"

class Path : public TimedInteractiveAlgorithm {
    Q_OBJECT

   public:
    Path(Graph* parent);
    ~Path();

    void start(Node* start);
    void setSourceNodeIndex(size_t sourceNode);
    void showPseudocode() override;

   private:
    bool stepOnce() override;
    void stepAll() override;

    void unmarkAllButUnvisitedNodes();

    GenericTraversal* m_genericTraversal;
    size_t m_sourceNode{std::numeric_limits<size_t>::max()};

    Node* m_currentNode{nullptr};
};

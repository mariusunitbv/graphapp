#pragma once

#include "../traversals/DepthTotalTraversal.h"

class StronglyConnectedComponents : public Algorithm {
    Q_OBJECT

   public:
    class CustomDepthTotalTraversal : public DepthTraversal {
       public:
        friend class StronglyConnectedComponents;

        CustomDepthTotalTraversal(Graph* parent, DepthTraversal* rhs);

        Graph* getMergedGraph() const;

       protected:
        bool stepOnce() override;
        void stepAll() override;

       private:
        bool pickAnotherNode();
        Node* getNextNodeToAnalyze();

        std::vector<std::pair<size_t, size_t>> m_sortedAnalyzeFinishOrder;

        std::vector<Node*> m_currentComponentNodes;
        std::vector<std::vector<Node*>> m_stronglyConnectedComponents;

        size_t m_latestStackSize{0};
    };

    StronglyConnectedComponents(Graph* parent);

    void showPseudocode() override;
    void start();
    void setStepDelay(int stepDelay);

   private:
    void onTraversalFinished();
    void beginInvertedTraversal();
    void onInvertedTraversalFinished();

    DepthTotalTraversal* m_traversal{nullptr};
    CustomDepthTotalTraversal* m_traversalOfInverted{nullptr};

    Graph* m_invertedGraph{nullptr};
    int m_stepDelay{0};
};

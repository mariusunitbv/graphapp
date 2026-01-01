#pragma once

#include "../Graph.h"

#include "../form/pseudocode/PseudocodeForm.h"

class IAlgorithm : public QObject {
    Q_OBJECT

   public:
    explicit IAlgorithm(Graph* graph);
    virtual ~IAlgorithm();

    virtual void start() = 0;

    void setNodeState(NodeIndex_t nodeIndex, NodeData::State state);
    NodeData::State getNodeState(NodeIndex_t nodeIndex) const;

    virtual void cancelAlgorithm();
    virtual void onFinishedAlgorithm();
    virtual void showPseudocodeForm();

   signals:
    void finished();
    void aborted();

    void pickedStartNode(NodeIndex_t nodeIndex);
    void visitedNode(NodeIndex_t nodeIndex);
    void analyzingNode(NodeIndex_t nodeIndex);
    void analyzedNode(NodeIndex_t nodeIndex);

   protected:
    Graph* m_graph{nullptr};
    PseudocodeForm m_pseudocodeForm;

    bool m_cancelRequested{false};
};

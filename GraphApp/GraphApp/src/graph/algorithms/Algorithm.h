#pragma once

#include "../graph/Graph.h"

#include "../form/pseudocode/PseudocodeForm.h"

class Algorithm : public QObject {
    Q_OBJECT

   public:
    virtual void showPseudocode();

   signals:
    void finished();
    void aborted();

   protected:
    Algorithm(Graph* parent);
    virtual ~Algorithm() = default;

    virtual void onEscapePressed();
    virtual void onFinishedAlgorithm();

    Graph* m_graph;
    PseudocodeForm m_pseudocodeForm;
};

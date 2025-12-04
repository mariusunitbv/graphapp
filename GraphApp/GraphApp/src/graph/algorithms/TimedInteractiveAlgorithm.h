#pragma once

#include "../graph/Graph.h"

#include "../form/pseudocode/PseudocodeForm.h"

class TimedInteractiveAlgorithm : public QObject {
    Q_OBJECT

   public:
    virtual bool stepOnce() = 0;
    virtual void stepAll() = 0;
    virtual void showPseudocode();

    void setStepDelay(int stepDelay);

   signals:
    void ticked();
    void finished();

   protected:
    TimedInteractiveAlgorithm(Graph* parent);
    virtual ~TimedInteractiveAlgorithm() = default;

    /**
     * @brief Handles a Space key press.
     *
     * Calling this function forces the step timer to time out,
     * allowing the algorithm to advance by one step.
     */
    void onSpacePressed();

    Graph* m_graph;
    PseudocodeForm m_pseudocodeForm;

    QTimer m_stepTimer;
    int m_stepDelay{0};
};

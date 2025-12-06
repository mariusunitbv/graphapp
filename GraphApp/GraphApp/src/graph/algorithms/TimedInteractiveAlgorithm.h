#pragma once

#include "Algorithm.h"

class TimedInteractiveAlgorithm : public Algorithm {
    Q_OBJECT

   public:
    virtual bool stepOnce() = 0;
    virtual void stepAll() = 0;

    void setStepDelay(int stepDelay);

   signals:
    void ticked();

   protected:
    TimedInteractiveAlgorithm(Graph* parent);

    /**
     * @brief Handles a Space key press.
     *
     * Calling this function forces the step timer to time out,
     * allowing the algorithm to advance by one step.
     */
    void onSpacePressed();
    void onEscapePressed() override;
    void onFinishedAlgorithm();

    QTimer m_stepTimer;
    QMetaObject::Connection m_stepConnection;

    int m_stepDelay{0};
};

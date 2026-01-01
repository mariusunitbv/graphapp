#pragma once

#include "IAlgorithm.h"

class ITimedAlgorithm : public IAlgorithm {
    Q_OBJECT

   public:
    ITimedAlgorithm(Graph* graph);

    void start() override;

    virtual bool step() = 0;
    void stepAll();

   protected:
    void markAllNodesUnvisited();
    void unmarkAllNodes();

    void onTimerTimeout();
    void onSpacePressed();
    void cancelAlgorithm() override;
    void onFinishedAlgorithm() override;

    virtual void updateAlgorithmInfoText() const = 0;

    int m_stepDelay{1000};
    int m_iterationsPerStep{1};

    QTimer m_stepTimer;
    QMetaObject::Connection m_stepConnection;
};

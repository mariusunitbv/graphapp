#pragma once

#include "ui_GraphApp.h"

class GraphApp : public QMainWindow {
    Q_OBJECT

   public:
    GraphApp(QWidget* parent = nullptr);

   private:
    void onZoomChanged();
    void onStartedAlgorithm();
    void onFinishedAlgorithm();
    void onEndedAlgorithm();

    bool openStepDurationDialog();
    bool openPathSourceNodeDialog();

    Ui::GraphAppClass ui;

    int m_stepDelay{1000};
    size_t m_sourceNodePath{std::numeric_limits<size_t>::max()};
};

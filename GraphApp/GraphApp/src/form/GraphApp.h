#pragma once

#include "ui_GraphApp.h"

class GraphApp : public QMainWindow {
    Q_OBJECT

   public:
    GraphApp(QWidget* parent = nullptr);
    ~GraphApp();

   private:
    void onZoomChanged();
    void onStartedAlgorithm();
    void onEndedAlgorithm();

    bool openStepDurationDialog();
    bool openPathSourceNodeDialog();

    Ui::GraphAppClass ui;
    int m_stepDelay;

    size_t m_sourceNodePath;

    QGraphicsScene* m_scene;
};

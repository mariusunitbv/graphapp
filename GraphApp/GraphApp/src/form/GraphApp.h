#pragma once

#include "ui_GraphApp.h"

class Graph;

class GraphApp : public QMainWindow {
    Q_OBJECT

   public:
    GraphApp(QWidget* parent = nullptr);

    void setGraph(Graph* graph);

   private:
    void onStartedAlgorithm();
    void onFinishedAlgorithm();
    void onEndedAlgorithm();

    Ui::GraphAppClass ui;
};

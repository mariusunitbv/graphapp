#pragma once

#include "ui_GraphApp.h"

class Graph;

class GraphApp : public QMainWindow {
    Q_OBJECT

   public:
    GraphApp(QWidget* parent = nullptr);

    void setGraph(Graph* graph);

   protected:
    void keyPressEvent(QKeyEvent* event) final;

   private:
    void onStartedAlgorithm();
    void onFinishedAlgorithm();
    void onEndedAlgorithm();

    void saveGraph();
    void loadGraph();

    Ui::GraphAppClass ui;
};

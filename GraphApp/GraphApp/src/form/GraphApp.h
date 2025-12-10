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

    Ui::GraphAppClass ui;
};

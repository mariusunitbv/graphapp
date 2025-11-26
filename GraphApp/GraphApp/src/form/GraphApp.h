#pragma once

#include "ui_GraphApp.h"

class GraphApp : public QMainWindow {
    Q_OBJECT

   public:
    GraphApp(QWidget* parent = nullptr);
    ~GraphApp();

   private:
    void onZoomChanged();

    Ui::GraphAppClass ui;

    QGraphicsScene* m_scene;
};

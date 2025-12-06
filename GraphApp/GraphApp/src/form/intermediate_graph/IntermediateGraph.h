#pragma once

#include "ui_IntermediateGraph.h"

#include "../graph/Graph.h"

class IntermediateGraph : public QMainWindow {
    Q_OBJECT

   public:
    IntermediateGraph(Graph* graph, QWidget* parent = nullptr);

   private:
    Ui::IntermediateGraphClass ui;
};

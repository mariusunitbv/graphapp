#pragma once

#include "ui_AdjacencyListBuilder.h"

#include "../graph/Graph.h"

class AdjacencyListBuilder : public QDialog {
    Q_OBJECT

   public:
    AdjacencyListBuilder(Graph* graph, QWidget* parent = nullptr);

   private:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void buildAdjacencyFromInput();

    Ui::AdjacencyListBuilderClass ui;

    Graph* m_graph;
};

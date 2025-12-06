#include <pch.h>

#include "IntermediateGraph.h"

IntermediateGraph::IntermediateGraph(Graph* graph, QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    connect(graph, &Graph::escapePressed, this, &QMainWindow::close);

    setCentralWidget(graph);
}

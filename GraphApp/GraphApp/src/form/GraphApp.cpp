#include <pch.h>

#include "GraphApp.h"

#include "scene_size/SceneSizeInput.h"
#include "adjacency_list/AdjacencyListBuilder.h"
#include "intermediate_graph/IntermediateGraph.h"

#include "../graph/pbf/PBFLoader.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);
    ui.graph->getGraphManager().setAllowEditing(true);

    connect(ui.actionComplete_Graph, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().completeGraph(); });

    connect(ui.actionBuild_Adjacency_List, &QAction::triggered, [this]() {
        AdjacencyListBuilder dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            ui.graph->buildFromAdjacencyListString(dialog.getAdjacencyListText());
            ui.graph->getGraphManager().buildVisibleEdgeCache();
        }
    });

    connect(ui.actionRandom_Graph, &QAction::triggered, [this]() {
        bool ok = false;

        auto& graphManager = ui.graph->getGraphManager();

        const auto maxEdges = graphManager.getMaxEdgesCount();
        const auto edgeCount = QInputDialog::getInt(
            nullptr, "Edge Count", QString("Number of edges (max %1):").arg(maxEdges), maxEdges / 3,
            0, maxEdges, 1, &ok);

        if (!ok || edgeCount == 0) {
            return;
        }

        graphManager.randomlyAddEdges(edgeCount);
    });

    connect(ui.actionBuild_Visible_Edge_Cache, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().buildVisibleEdgeCache(); });

    connect(ui.actionBuild_Full_Edge_Cache, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().buildFullEdgeCache(); });

    connect(ui.actionAnimations, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setAnimationsDisabled(!checked); });

    connect(ui.actionAllow_Editing, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setAllowEditing(checked); });

    connect(ui.actionAllow_Loops, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setAllowLoops(checked); });

    connect(ui.actionOriented_Graph, &QAction::toggled, [this](bool checked) {
        ui.graph->getGraphManager().setOrientedGraph(checked);
        ui.actionAllow_Loops->setEnabled(checked);

        if (!checked) {
            ui.actionAllow_Loops->setChecked(false);
        }
    });

    connect(ui.actionReset_Graph, &QAction::triggered, [this]() {
        const auto response =
            QMessageBox::warning(this, "Confirmation", "Are you sure you want to reset the graph?",
                                 QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            ui.graph->getGraphManager().reset();
            ui.graph->getGraphManager().update();
        }
    });

    connect(ui.actionInverted_Graph, &QAction::triggered, this, [this]() {
        Graph* invertedGraph = ui.graph->getInvertedGraph();
        if (!invertedGraph) {
            return;
        }

        const auto window = new IntermediateGraph(invertedGraph, this);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });

    connect(ui.actionGenericTraversal, &QAction::triggered, [this]() {

    });

    connect(ui.actionGeneric_Total_Traversal, &QAction::triggered, [this]() {

    });

    connect(ui.actionPath_s_y, &QAction::triggered, [this]() {

    });

    connect(ui.actionBreadth_Traversal, &QAction::triggered, [this]() {

    });

    connect(ui.actionDepth_Traversal, &QAction::triggered, [this]() {

    });

    connect(ui.actionDepth_Total_Traversal, &QAction::triggered, [this]() {

    });

    connect(ui.actionConnected_Components, &QAction::triggered, [this]() {

    });

    connect(ui.actionStrongly_Connected_Components, &QAction::triggered, [this]() {

    });

    connect(ui.actionDijkstra_s_algorithm, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().dijkstra(); });

    connect(ui.actionFill_Graph_With_Nodes, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().fillGraph(); });

    connect(ui.actionChange_Scene_Dimensions, &QAction::triggered, [this]() {
        SceneSizeInput dialog(ui.graph->getSceneSize(), this);
        if (dialog.exec() == QDialog::Accepted) {
            ui.graph->setSceneSize(dialog.getEnteredSize());
        }
    });

    connect(ui.action_Custom_Load_Map, &QAction::triggered, [this]() {
        const auto filePath =
            QFileDialog::getOpenFileName(this, "Open Map File", "", "PBF Files (*.pbf)");
        if (filePath.isEmpty()) {
            return;
        }

        PBFLoader loader(&ui.graph->getGraphManager(), filePath);
        loader.tryLoad();

        ui.actionAllow_Editing->setChecked(ui.graph->getGraphManager().getAllowEditing());
    });

    connect(ui.actionDraw_Nodes, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawNodesEnabled(checked); });

    connect(ui.actionDraw_Edges, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawEdgesEnabled(checked); });

    connect(ui.actionDraw_Quad_Trees, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawQuadTreesEnabled(checked); });

    connect(ui.actionCenter_on_Node, &QAction::triggered, [this]() {
        auto& graphManager = ui.graph->getGraphManager();
        const auto nodeCount = static_cast<NodeIndex_t>(graphManager.getNodesCount());
        if (nodeCount == 0) {
            QMessageBox::information(nullptr, "Center on Node", "The graph has no nodes.",
                                     QMessageBox::Ok);
            return;
        }

        bool ok = false;
        const auto nodeIndex = QInputDialog::getInt(
            nullptr, "Node", "Enter the Node to center on:", 0, 0, nodeCount - 1, 1, &ok);

        if (!ok) {
            return;
        }

        const auto nodePos = graphManager.getNode(nodeIndex).getPosition();
        ui.graph->centerOn(nodePos);
    });

    connect(ui.actionToggle_Light_Dark_Mode, &QAction::triggered,
            [this]() { ui.graph->toggleDarkMode(); });
}

void GraphApp::onStartedAlgorithm() {
    ui.menuBar->setEnabled(false);
    ui.graph->getGraphManager().setAllowEditing(false);
}

void GraphApp::onFinishedAlgorithm() {
    QMessageBox::information(
        nullptr, "Algorithm",
        "The aglorithm has finished\nPress escape to return to the initial state.",
        QMessageBox::Ok);
}

void GraphApp::onEndedAlgorithm() {
    ui.menuBar->setEnabled(true);
    ui.graph->getGraphManager().setAllowEditing(true);
}

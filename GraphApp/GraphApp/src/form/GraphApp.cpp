#include <pch.h>

#include "GraphApp.h"

#include "intermediate_graph/IntermediateGraph.h"

#include "../dependencies/tinyxml2.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    ui.graph->getGraphManager().setAllowEditing(true);

    connect(ui.textEdit, &QTextEdit::textChanged,
            [this]() { ui.graph->onAdjacencyListChanged(ui.textEdit->toPlainText()); });
    connect(ui.graph, &Graph::zoomChanged, this, &GraphApp::onZoomChanged);
    connect(ui.actionComplete_Graph, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().completeGraph(); });

    connect(ui.actionRandom_Graph, &QAction::triggered, [this]() {
        bool ok = false;

        auto& graphManager = ui.graph->getGraphManager();

        const auto nodesCount = graphManager.getNodesCount();
        const auto maxEdges = graphManager.getMaxEdgesCount();

        const auto edgeCount = QInputDialog::getInt(
            nullptr, "Edge Count", QString("Number of edges (max %1):").arg(maxEdges), maxEdges / 3,
            0, maxEdges, 1, &ok);

        if (!ok || edgeCount == 0) {
            return;
        }

        graphManager.randomlyAddEdges(edgeCount);
    });

    connect(ui.actionBuild_Full_Edge_Cache, &QAction::triggered, [this]() {
        const auto response = QMessageBox::information(nullptr, "Confirmation",
                                                       "Building the full edge cache may take a "
                                                       "long time for large graphs. Do you want "
                                                       "to proceed?",
                                                       QMessageBox::Yes, QMessageBox::No);
        if (response == QMessageBox::Yes) {
            ui.graph->getGraphManager().buildFullEdgeCache();
        }
    });

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
            ui.textEdit->clear();
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

    connect(ui.action_Custom_Load_Map, &QAction::triggered, [this]() {
        const auto filePath =
            QFileDialog::getOpenFileName(this, "Open Map File", "", "All Files (*)");
        if (filePath.isEmpty()) {
            return;
        }

        auto& graphManager = ui.graph->getGraphManager();
        const auto sceneRect = ui.graph->sceneRect().toRect();

        graphManager.reset();
        graphManager.setCollisionsCheckEnabled(false);

        ui.actionAllow_Editing->setChecked(false);

        try {
            qreal minLat = std::numeric_limits<qreal>::max(),
                  minLon = std::numeric_limits<qreal>::max();
            qreal maxLat = std::numeric_limits<qreal>::min(),
                  maxLon = std::numeric_limits<qreal>::min();

            using nlohmann::json;
            const auto j = json::parse(std::ifstream(filePath.toStdString()));

            for (const auto& feature : j["features"]) {
                if (feature["geometry"]["type"] != "LineString") {
                    continue;
                }

                const auto& coords = feature["geometry"]["coordinates"];
                for (const auto& coord : coords) {
                    const qreal lon = coord[0];
                    const qreal lat = coord[1];

                    if (lat < minLat) {
                        minLat = lat;
                    }

                    if (lat > maxLat) {
                        maxLat = lat;
                    }

                    if (lon < minLon) {
                        minLon = lon;
                    }

                    if (lon > maxLon) {
                        maxLon = lon;
                    }
                }
            }

            const int centerX = sceneRect.width() / 2;
            const int centerY = sceneRect.height() / 2;
            constexpr auto scale = 1;

            QHash<QPoint, NodeIndex_t> loadedNodes;
            for (const auto& feature : j["features"]) {
                if (feature["geometry"]["type"] != "LineString") {
                    continue;
                }

                const auto& coords = feature["geometry"]["coordinates"];
                for (const auto& coord : coords) {
                    const double lon = coord[0];
                    const double lat = coord[1];

                    const double normLon = (lon - minLon) / (maxLon - minLon);
                    const double normLat = (lat - minLat) / (maxLat - minLat);

                    const int x = centerX + (normLon - 0.5) * scale *
                                                (sceneRect.width() - 2 * NodeData::k_radius);
                    const int y = centerY + (0.5 - normLat) * scale *
                                                (sceneRect.height() - 2 * NodeData::k_radius);

                    if (!loadedNodes.contains({x, y})) {
                        graphManager.addNode({x, y});
                        loadedNodes[{x, y}] = graphManager.getNodesCount() - 1;
                    }
                }
            }

            graphManager.resizeAdjacencyMatrix(graphManager.getNodesCount());

            for (const auto& feature : j["features"]) {
                if (feature["geometry"]["type"] != "LineString") {
                    continue;
                }

                const bool oneWay = feature["properties"].value("oneway", "no") == "yes";

                NodeIndex_t prevNodeIndex = INVALID_NODE;
                const auto& coords = feature["geometry"]["coordinates"];
                for (const auto& coord : coords) {
                    const double lon = coord[0];
                    const double lat = coord[1];

                    const double normLon = (lon - minLon) / (maxLon - minLon);
                    const double normLat = (lat - minLat) / (maxLat - minLat);

                    const int x = centerX + (normLon - 0.5) * scale *
                                                (sceneRect.width() - 2 * NodeData::k_radius);
                    const int y = centerY + (0.5 - normLat) * scale *
                                                (sceneRect.height() - 2 * NodeData::k_radius);

                    const auto nodeIndex = loadedNodes.value({x, y}, INVALID_NODE);
                    if (nodeIndex == INVALID_NODE) {
                        throw std::runtime_error("Node not found during edge creation.");
                    }

                    if (prevNodeIndex == INVALID_NODE) {
                        prevNodeIndex = nodeIndex;
                        continue;
                    }

                    const auto currentNodeIndex = nodeIndex;
                    graphManager.addEdge(prevNodeIndex, currentNodeIndex, 1);
                    if (!oneWay) {
                        graphManager.addEdge(currentNodeIndex, prevNodeIndex, 1);
                    }

                    prevNodeIndex = currentNodeIndex;
                }
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error",
                                 QString("Failed to load the map file:\n%1").arg(e.what()));
            graphManager.reset();
            return;
        }

        graphManager.buildFullEdgeCache();
        graphManager.setCollisionsCheckEnabled(true);
    });

    connect(ui.actionDraw_Nodes, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawNodesEnabled(checked); });

    connect(ui.actionDraw_Edges, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawEdgesEnabled(checked); });

    connect(ui.actionDraw_Quad_Trees, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawQuadTreesEnabled(checked); });

    connect(ui.actionToggle_Light_Dark_Mode, &QAction::triggered,
            [this]() { ui.graph->toggleDarkMode(); });
}

void GraphApp::onZoomChanged() {
    ui.zoomScaleText->setText(QString("Graph scale: %1%").arg(ui.graph->getZoomPercentage()));
}

void GraphApp::onStartedAlgorithm() {
    ui.menuBar->setEnabled(false);
    ui.textEdit->setEnabled(false);
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
    ui.textEdit->setEnabled(true);
    ui.graph->getGraphManager().setAllowEditing(true);
}

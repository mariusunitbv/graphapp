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

    connect(ui.actionFill_Graph_With_Nodes, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().fillGraph(); });

    connect(ui.action_Custom_Load_Map, &QAction::triggered, [this]() {
        const auto filePath =
            QFileDialog::getOpenFileName(this, "Open Map File", "", "All Files (*)");
        if (filePath.isEmpty()) {
            return;
        }

        using namespace tinyxml2;

        XMLDocument doc;
        if (doc.LoadFile(filePath.toStdString().c_str()) != XML_SUCCESS) {
            QMessageBox::warning(this, "Error", "Failed to load the map file.");
            return;
        }

        auto& graphManager = ui.graph->getGraphManager();
        const auto sceneRect = ui.graph->sceneRect().toRect();

        graphManager.reset();
        graphManager.setCollisionsCheckEnabled(false);

        std::vector<QPoint> loadedNodes;
        int minLat = std::numeric_limits<int>::max(), minLon = std::numeric_limits<int>::max();
        int maxLat = std::numeric_limits<int>::min(), maxLon = std::numeric_limits<int>::min();

        XMLElement* map = doc.FirstChildElement("map");
        XMLElement* nodesElement = map->FirstChildElement("nodes");

        loadedNodes.reserve(nodesElement->ChildElementCount());

        for (XMLElement* nodeElem = nodesElement->FirstChildElement("node"); nodeElem;
             nodeElem = nodeElem->NextSiblingElement("node")) {
            NodeIndex_t id;
            int lat, lon;

            nodeElem->QueryUnsignedAttribute("id", &id);
            nodeElem->QueryIntAttribute("latitude", &lat);
            nodeElem->QueryIntAttribute("longitude", &lon);

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

            loadedNodes.emplace_back(lat, lon);
        }

        const int centerX = sceneRect.width() / 2;
        const int centerY = sceneRect.height() / 2;
        constexpr auto scale = 1;

        graphManager.reserveNodes(loadedNodes.size());
        for (const auto& pos : loadedNodes) {
            const double normX = static_cast<double>(pos.y() - minLon) / (maxLon - minLon);
            const double normY = static_cast<double>(pos.x() - minLat) / (maxLat - minLat);

            const int x =
                centerX + (normX - 0.5) * scale * (sceneRect.width() - 2 * NodeData::k_radius);
            const int y =
                centerY + (normY - 0.5) * scale * (sceneRect.height() - 2 * NodeData::k_radius);

            graphManager.addNode({x, y});
        }

        graphManager.resizeAdjacencyMatrix(graphManager.getNodesCount());

        XMLElement* arcsElement = map->FirstChildElement("arcs");
        for (XMLElement* arcElem = arcsElement->FirstChildElement("arc"); arcElem;
             arcElem = arcElem->NextSiblingElement("arc")) {
            NodeIndex_t from, to;
            int32_t cost;

            arcElem->QueryUnsignedAttribute("from", &from);
            arcElem->QueryUnsignedAttribute("to", &to);
            arcElem->QueryIntAttribute("length", &cost);

            graphManager.addEdge(from, to, cost);
        }

        graphManager.updateVisibleEdgeCache();
        graphManager.setCollisionsCheckEnabled(true);
    });

    connect(ui.actionDraw_Nodes, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawNodesEnabled(checked); });

    connect(ui.actionDraw_Edges, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawEdgesEnabled(checked); });

    connect(ui.actionDraw_Quad_Trees, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawQuadTreesEnabled(checked); });
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

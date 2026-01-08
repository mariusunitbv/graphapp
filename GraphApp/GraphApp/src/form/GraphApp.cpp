#include <pch.h>

#include "GraphApp.h"

#include "scene_size/SceneSizeInput.h"
#include "adjacency_list/AdjacencyListBuilder.h"

#include "../graph/algorithms/traversals/GenericTraversal.h"
#include "../graph/algorithms/traversals/GenericTotalTraversal.h"
#include "../graph/algorithms/traversals/BreadthFirstTraversal.h"
#include "../graph/algorithms/traversals/DepthFirstTraversal.h"
#include "../graph/algorithms/traversals/DepthFirstTotalTraversal.h"

#include "../graph/algorithms/paths/PathReconstruction.h"

#include "../graph/algorithms/custom/TopologicalSort.h"
#include "../graph/algorithms/custom/ConnectedComponents.h"
#include "../graph/algorithms/custom/StronglyConnectedComponents.h"

#include "../graph/pbf/PBFLoader.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);
    ui.graph->getGraphManager().setAllowEditing(true);

    connect(ui.actionBuild_Adjacency_List, &QAction::triggered, [this]() {
        auto& graphManager = ui.graph->getGraphManager();
        if (graphManager.getNodesCount() == 0) {
            QMessageBox::warning(this, "Adjacency Builder", "There are no nodes in the graph!");
            return;
        }

        if (!graphManager.getAllowEditing()) {
            QMessageBox::warning(this, "Adjacency Builder",
                                 "Graph editing is disabled! Enable it to build adjacency list.");
            return;
        }

        AdjacencyListBuilder dialog(ui.graph, this);
        if (dialog.exec() == QDialog::Accepted) {
            graphManager.markEdgesDirty();
            graphManager.buildEdgeCache();
        }
    });

    connect(ui.actionRandom_Graph, &QAction::triggered, [this]() {
        auto& graphManager = ui.graph->getGraphManager();
        if (graphManager.getNodesCount() == 0) {
            QMessageBox::warning(this, "Random Fill Adjacency List",
                                 "There are no nodes in the graph!");
            return;
        }

        if (!graphManager.getAllowEditing()) {
            QMessageBox::warning(this, "Random Fill Adjacency List",
                                 "Graph editing is disabled! Enable it to build adjacency list.");
            return;
        }

        bool ok = false;
        const auto maxEdges = graphManager.getMaxEdgesCount();

        const auto edgeCount = QInputDialog::getText(
            nullptr, "Edge Count", QString("Number of edges (max %1):").arg(maxEdges),
            QLineEdit::Normal, QString::number(maxEdges / 3), &ok);
        if (!ok) {
            return;
        }

        const auto edgeCountInt = edgeCount.toULongLong(&ok);
        if (!ok || edgeCountInt < 1 || static_cast<qulonglong>(edgeCountInt) > maxEdges) {
            QMessageBox::warning(this, "Invalid Input", "The entered edge count is invalid.");
            return;
        }

        if (edgeCountInt == maxEdges) {
            return graphManager.completeGraph();
        }

        graphManager.resetAdjacencyMatrix();
        graphManager.resizeAdjacencyMatrix(graphManager.getNodesCount());

        graphManager.evaluateStorageStrategy(edgeCountInt);
        graphManager.randomlyAddEdges(edgeCountInt);
    });

    connect(ui.actionFill_Graph_With_Nodes, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().fillGraph(); });

    connect(ui.actionReset_Graph, &QAction::triggered, [this]() {
        const auto response =
            QMessageBox::warning(this, "Confirmation", "Are you sure you want to reset the graph?",
                                 QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            ui.graph->getGraphManager().reset();
            ui.graph->getGraphManager().update();
        }
    });

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

    connect(ui.actionSave_Graph, &QAction::triggered, this, &GraphApp::saveGraph);

    connect(ui.actionLoad_Graph, &QAction::triggered, this, &GraphApp::loadGraph);

    connect(ui.actionInverted_Graph_2, &QAction::triggered, this, [this]() {
        Graph* invertedGraph = ui.graph->getInvertedGraph();
        if (!invertedGraph) {
            return;
        }

        const auto window = new GraphApp(nullptr);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setGraph(invertedGraph);
        window->show();
    });

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
        ui.actionDraw_Nodes->setChecked(ui.graph->getGraphManager().getDrawNodesEnabled());
        ui.actionOriented_Graph->setChecked(ui.graph->getGraphManager().getOrientedGraph());
    });

    connect(ui.actionGeneric, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto traversal = new GenericTraversal(ui.graph);
        connect(traversal, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(traversal, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        traversal->showPseudocodeForm();
        traversal->start(selectedNodeOpt.value());
    });

    connect(ui.actionGeneric_Total_Traversal_2, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto traversal = new GenericTotalTraversal(ui.graph);
        connect(traversal, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(traversal, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        traversal->showPseudocodeForm();
        traversal->start(selectedNodeOpt.value());
    });

    connect(ui.actionPath, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(this, "Warning",
                                 "No node is selected! Please select a node to start the traversal "
                                 "for the path reconstructing algorithm.");
            return;
        }

        onStartedAlgorithm();

        const auto path = new PathReconstruction(ui.graph);
        connect(path, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(path, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        path->showPseudocodeForm();
        path->start(selectedNodeOpt.value());
    });

    connect(ui.actionBreadth_Traversal_2, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto traversal = new BreadthFirstTraversal(ui.graph);
        connect(traversal, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(traversal, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        traversal->showPseudocodeForm();
        traversal->start(selectedNodeOpt.value());
    });

    connect(ui.actionDepth_Traversal_2, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto traversal = new DepthFirstTraversal(ui.graph);
        connect(traversal, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(traversal, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        traversal->showPseudocodeForm();
        traversal->start(selectedNodeOpt.value());
    });

    connect(ui.actionDepth_Total_Traversal_2, &QAction::triggered, [this]() {
        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto traversal = new DepthFirstTotalTraversal(ui.graph);
        connect(traversal, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(traversal, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        traversal->showPseudocodeForm();
        traversal->start(selectedNodeOpt.value());
    });

    connect(ui.actionTopological_Sort, &QAction::triggered, [this]() {
        if (!ui.graph->getGraphManager().getOrientedGraph()) {
            QMessageBox::warning(
                this, "Warning",
                "The graph is not oriented! Topological sort can only be performed on oriented "
                "graphs.");
            return;
        }

        const auto selectedNodeOpt = ui.graph->getGraphManager().getSelectedNode();
        if (!selectedNodeOpt) {
            QMessageBox::warning(
                this, "Warning",
                "No node is selected! Please select a node to start the traversal.");
            return;
        }

        onStartedAlgorithm();

        const auto topologicalSort = new TopologicalSort(ui.graph);
        connect(topologicalSort, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(topologicalSort, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        topologicalSort->showPseudocodeForm();
        topologicalSort->start(selectedNodeOpt.value());
    });

    connect(ui.actionConnected_Components_2, &QAction::triggered, [this]() {
        if (ui.graph->getGraphManager().getNodesCount() == 0) {
            QMessageBox::warning(this, "Warning", "The graph has no nodes!");
            return;
        }

        if (ui.graph->getGraphManager().getOrientedGraph()) {
            QMessageBox::warning(
                this, "Warning",
                "The graph is oriented! Connected components can only be found in non-oriented "
                "graphs.");
            return;
        }

        onStartedAlgorithm();

        const auto cc = new ConnectedComponents(ui.graph);
        connect(cc, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(cc, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        cc->showPseudocodeForm();
        cc->start();
    });

    connect(ui.actionStrongly_Connected_Components_2, &QAction::triggered, [this]() {
        if (ui.graph->getGraphManager().getNodesCount() == 0) {
            QMessageBox::warning(this, "Warning", "The graph has no nodes!");
            return;
        }

        if (!ui.graph->getGraphManager().getOrientedGraph()) {
            QMessageBox::warning(this, "Warning",
                                 "The graph is not oriented! Strongly Connected Components can "
                                 "only be performed on oriented "
                                 "graphs.");
            return;
        }

        onStartedAlgorithm();

        const auto ctc = new StronglyConnectedComponents(ui.graph);
        connect(ctc, &IAlgorithm::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(ctc, &IAlgorithm::aborted, this, &GraphApp::onEndedAlgorithm);

        ctc->showPseudocodeForm();
        ctc->start();
    });

    connect(ui.actionDijkstra_s_algorithm, &QAction::triggered,
            [this]() { ui.graph->getGraphManager().dijkstra(); });

    connect(ui.actionDraw_Nodes, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawNodesEnabled(checked); });

    connect(ui.actionDraw_Edges, &QAction::toggled,
            [this](bool checked) { ui.graph->getGraphManager().setDrawEdgesEnabled(checked); });

    connect(ui.actionRefresh_Edges_Cache, &QAction::triggered, [this]() {
        ui.graph->getGraphManager().markEdgesDirty();
        ui.graph->getGraphManager().buildEdgeCache();
    });

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

        graphManager.updateVisibleSceneRect();
        graphManager.buildEdgeCache();
    });

    connect(ui.actionToggle_Light_Dark_Mode, &QAction::triggered,
            [this]() { ui.graph->toggleDarkMode(); });
}

void GraphApp::setGraph(Graph* graph) {
    delete ui.graph;
    graph->setParent(this);
    ui.gridLayout->addWidget(graph);
    ui.graph = graph;
    ui.actionAllow_Editing->setChecked(graph->getGraphManager().getAllowEditing());

    ui.graph->getGraphManager().buildEdgeCache();
}

void GraphApp::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F) {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    }
}

void GraphApp::closeEvent(QCloseEvent* event) {
    ui.graph->getGraphManager().cancelAlgorithms();
    QCoreApplication::processEvents();
    event->accept();
}

void GraphApp::onStartedAlgorithm() {
    ui.menuGraph->setEnabled(false);
    ui.menuTraversals->setEnabled(false);
    ui.graph->getGraphManager().setAllowEditing(false);
}

void GraphApp::onFinishedAlgorithm() {
    QMessageBox::information(
        nullptr, "Algorithm",
        "The algorithm has finished.\nPressing escape will return the graph to its initial state.",
        QMessageBox::Ok);
}

void GraphApp::onEndedAlgorithm() {
    ui.menuGraph->setEnabled(true);
    ui.menuTraversals->setEnabled(true);
    ui.graph->getGraphManager().setAllowEditing(true);
}

void GraphApp::saveGraph() {
    using namespace simdjson;

    auto& graphManager = ui.graph->getGraphManager();
    if (graphManager.getNodesCount() == 0) {
        QMessageBox::warning(this, "Save Graph", "There are no nodes in the graph!");
        return;
    }

    const auto filePath =
        QFileDialog::getSaveFileName(this, "Save Graph", "", "Graph Files (*.graph)");
    if (filePath.isEmpty()) {
        return;
    }

    builder::string_builder sb;
    sb.start_object();
    {
        const auto graphSize = ui.graph->getSceneSize();
        const auto nodeCount = graphManager.getNodesCount();
        const auto storageType = static_cast<int>(graphManager.getGraphStorage()->type());

        sb.append_key_value<"scene_width">(graphSize.width());
        sb.append_comma();
        sb.append_key_value<"scene_height">(graphSize.height());
        sb.append_comma();
        sb.append_key_value<"oriented">(graphManager.getOrientedGraph());
        sb.append_comma();
        sb.append_key_value<"allow_loops">(graphManager.getAllowLoops());
        sb.append_comma();
        sb.append_key_value<"node_count">(nodeCount);
        sb.append_comma();
        sb.append_key_value<"storage_type">(storageType);
        sb.append_comma();

        sb.escape_and_append_with_quotes("nodes");
        sb.append_colon();
        sb.start_array();
        {
            for (NodeIndex_t i = 0; i < nodeCount; ++i) {
                const auto& node = graphManager.getNode(i);
                const auto position = node.getPosition();

                sb.start_object();
                {
                    sb.append_key_value<"x">(position.x());
                    sb.append_comma();
                    sb.append_key_value<"y">(position.y());
                    sb.append_comma();

                    sb.escape_and_append_with_quotes("edges");
                    sb.append_colon();
                    sb.start_array();
                    {
                        bool first = true;
                        graphManager.getGraphStorage()->forEachOutgoingEdgeWithOpposites(
                            i, [&](NodeIndex_t j, CostType_t cost) {
                                if (!first) {
                                    sb.append_comma();
                                }
                                first = false;

                                sb.start_object();
                                {
                                    sb.append_key_value<"to">(j);
                                    if (cost != 0) {
                                        sb.append_comma();
                                        sb.append_key_value<"cost">(cost);
                                    }
                                }
                                sb.end_object();
                            });
                    }
                    sb.end_array();
                }
                sb.end_object();

                if (i < nodeCount - 1) {
                    sb.append_comma();
                }
            }
        }
        sb.end_array();
    }
    sb.end_object();

    std::ofstream outFile(filePath.toStdWString());
    outFile << sb.view();
}

void GraphApp::loadGraph() {
    using namespace simdjson;

    const auto filePath =
        QFileDialog::getOpenFileName(this, "Load Graph", "", "Graph Files (*.graph)");
    if (filePath.isEmpty()) {
        return;
    }

    auto& graphManager = ui.graph->getGraphManager();

    try {
        ondemand::parser parser;
        const auto json = padded_string::load(filePath.toStdWString());
        auto doc = parser.iterate(json);

        graphManager.setCollisionsCheckEnabled(false);

        const auto sceneWidth = doc["scene_width"].get_int64().value();
        const auto sceneHeight = doc["scene_height"].get_int64().value();

        ui.graph->setSceneSize(QSize(sceneWidth, sceneHeight));

        const auto oriented = doc["oriented"].get_bool().value();
        graphManager.setOrientedGraph(oriented);

        const auto allowLoops = doc["allow_loops"].get_bool().value();
        graphManager.setAllowLoops(allowLoops);

        const auto nodeCount = doc["node_count"].get_int64().value();
        const auto storageType =
            static_cast<IGraphStorage::Type>(doc["storage_type"].get_int64().value());
        graphManager.setGraphStorageType(storageType);
        graphManager.reserveNodes(nodeCount);
        graphManager.resizeAdjacencyMatrix(nodeCount);

        for (auto node : doc["nodes"]) {
            const auto x = node["x"].get_int64().value();
            const auto y = node["y"].get_int64().value();

            graphManager.addNode(QPoint(x, y));
            const auto addedNodeIndex = static_cast<NodeIndex_t>(graphManager.getNodesCount() - 1);

            for (auto neighbour : node["edges"]) {
                const auto neighbourIndex =
                    static_cast<NodeIndex_t>(neighbour["to"].get_int64().value());
                const auto cost = [&neighbour]() {
                    auto field = neighbour.find_field("cost");
                    if (field.error() == SUCCESS) {
                        return field.value().get_int64().value();
                    }

                    return 0ll;
                }();

                graphManager.getGraphStorage()->addEdge(addedNodeIndex, neighbourIndex, cost);
            }
        }

        graphManager.buildEdgeCache();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Load Graph",
                             QString("Failed to load graph: %1").arg(ex.what()));
    }

    graphManager.setCollisionsCheckEnabled(true);

    ui.actionOriented_Graph->setChecked(graphManager.getOrientedGraph());
    ui.actionAllow_Loops->setChecked(graphManager.getAllowLoops());
}

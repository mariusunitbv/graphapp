#include <pch.h>

#include "GraphApp.h"

#include "../random/Random.h"

#include "../graph/algorithms/traversals/GenericTraversal.h"
#include "../graph/algorithms/traversals/GenericTotalTraversal.h"
#include "../graph/algorithms/traversals/BreadthTraversal.h"
#include "../graph/algorithms/traversals/DepthTraversal.h"
#include "../graph/algorithms/traversals/DepthTotalTraversal.h"

#include "../graph/algorithms/other/ConnectedComponents.h"
#include "../graph/algorithms/other/StronglyConnectedComponents.h"

#include "../graph/algorithms/paths/Path.h"

#include "intermediate_graph/IntermediateGraph.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    ui.graph->enableEditing();

    connect(ui.textEdit, &QTextEdit::textChanged,
            [this]() { ui.graph->onAdjacencyListChanged(ui.textEdit->toPlainText()); });

    connect(ui.graph, &Graph::zoomChanged, this, &GraphApp::onZoomChanged);
    connect(ui.graph, &Graph::endedAlgorithm, this, &GraphApp::onEndedAlgorithm);

    connect(ui.actionComplete_Graph, &QAction::triggered, [this]() {
        auto& nodeManager = ui.graph->getNodeManager();
        if (ui.graph->getOrientedGraph()) {
            nodeManager.completeOrientedGraph(ui.graph->getAllowLoops());
        } else {
            nodeManager.completeUnorientedGraph();
        }
    });

    connect(ui.actionRandom_Graph, &QAction::triggered, [this]() {
        bool ok = false;
        const auto nodesCount = ui.graph->getNodesCount();
        const auto maxEdges = ui.graph->getMaxEdgesCount();

        const auto edgeCount = QInputDialog::getInt(
            nullptr, "Edge Count", QString("Number of edges (max %1):").arg(maxEdges), maxEdges / 3,
            0, maxEdges, 1, &ok);

        if (!ok || edgeCount == 0) {
            return;
        }

        std::vector<std::pair<size_t, size_t>> possibleEdges;
        for (size_t i = 0; i < nodesCount; ++i) {
            size_t j = ui.graph->getOrientedGraph() ? 0 : i + 1;
            for (; j < nodesCount; ++j) {
                if (i == j && !ui.graph->getAllowLoops()) {
                    continue;
                }
                possibleEdges.push_back({i, j});
            }
        }

        std::shuffle(possibleEdges.begin(), possibleEdges.end(), Random::get().getEngine());

        QString out;
        for (size_t k = 0; k < edgeCount; ++k) {
            out += QString("%1 %2\n").arg(possibleEdges[k].first).arg(possibleEdges[k].second);
        }

        ui.graph->reserveEdges(edgeCount);
        ui.textEdit->setText(out);
    });

    connect(ui.actionAnimations, &QAction::toggled,
            [this](bool checked) { ui.graph->setAnimationsDisabled(!checked); });

    connect(ui.actionAllow_Loops, &QAction::toggled,
            [this](bool checked) { ui.graph->setAllowLoops(checked); });

    connect(ui.actionOriented_Graph, &QAction::toggled, [this](bool checked) {
        ui.graph->setOrientedGraph(checked);
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
            ui.graph->getNodeManager().reset();
            ui.textEdit->clear();
        }
    });

    connect(ui.actionInverted_Graph, &QAction::triggered, this, [this]() {
        if (ui.graph->getEdges().empty()) {
            QMessageBox::warning(this, "No edges", "Cannot invert a graph with no edges.");
            return;
        }

        Graph* invertedGraph = ui.graph->getInvertedGraph();
        if (!invertedGraph) {
            return;
        }

        const auto window = new IntermediateGraph(invertedGraph, this);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });

    connect(ui.actionGenericTraversal, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Confirmation",
            QString("Begin algorithm from node %1?").arg(selectedNode->getIndex()),
            QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            if (!openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto traversal = new GenericTraversal(ui.graph);
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(traversal, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });

    connect(ui.actionGeneric_Total_Traversal, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Confirmation",
            QString("Begin algorithm from node %1?").arg(selectedNode->getIndex()),
            QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            if (!openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto traversal = new GenericTotalTraversal(ui.graph);
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(traversal, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });

    connect(ui.actionPath_s_y, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Information",
            QString("This algorithm involves a Generic Traversal starting from Node %1\nand "
                    "reconstructs a random path from an entered node to Node %1.")
                .arg(selectedNode->getIndex()),
            QMessageBox::Ok);

        if (response == QMessageBox::Ok) {
            if (!openPathSourceNodeDialog() || !openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto path = new Path(ui.graph);
            connect(path, &Path::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(path, &Path::aborted, this, &GraphApp::onEndedAlgorithm);

            path->setStepDelay(m_stepDelay);
            path->setSourceNodeIndex(m_sourceNodePath);
            path->showPseudocode();
            path->start(selectedNode);
        }
    });

    connect(ui.actionBreadth_Traversal, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Confirmation",
            QString("Begin algorithm from node %1?").arg(selectedNode->getIndex()),
            QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            if (!openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto traversal = new BreadthTraversal(ui.graph);
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(traversal, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });

    connect(ui.actionDepth_Traversal, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Confirmation",
            QString("Begin algorithm from node %1?").arg(selectedNode->getIndex()),
            QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            if (!openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto traversal = new DepthTraversal(ui.graph);
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(traversal, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });

    connect(ui.actionDepth_Total_Traversal, &QAction::triggered, [this]() {
        const auto selectedNode = ui.graph->getFirstSelectedNode();
        if (!selectedNode) {
            QMessageBox::warning(this, "No node selected",
                                 "Please select a starting node for the traversal.");
            return;
        }

        const auto response = QMessageBox::information(
            this, "Confirmation",
            QString("Begin algorithm from node %1?").arg(selectedNode->getIndex()),
            QMessageBox::Yes, QMessageBox::No);

        if (response == QMessageBox::Yes) {
            if (!openStepDurationDialog()) {
                return;
            }

            onStartedAlgorithm();

            const auto traversal = new DepthTotalTraversal(ui.graph);
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
            connect(traversal, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });

    connect(ui.actionConnected_Components, &QAction::triggered, [this]() {
        if (ui.graph->getNodesCount() == 0) {
            QMessageBox::warning(this, "No nodes", "The graph has no nodes.");
            return;
        }

        if (ui.graph->getOrientedGraph()) {
            QMessageBox::warning(this, "Oriented graph",
                                 "Connected Components algorithm is available only for "
                                 "non-oriented graphs.");
            return;
        }

        if (!openStepDurationDialog()) {
            return;
        }

        onStartedAlgorithm();

        const auto cc = new ConnectedComponents(ui.graph);
        connect(cc, &TraversalBase::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(cc, &TraversalBase::aborted, this, &GraphApp::onEndedAlgorithm);

        cc->setStepDelay(m_stepDelay);
        cc->showPseudocode();
        cc->start();
    });

    connect(ui.actionStrongly_Connected_Components, &QAction::triggered, [this]() {
        if (ui.graph->getNodesCount() == 0) {
            QMessageBox::warning(this, "No nodes", "The graph has no nodes.");
            return;
        }

        if (!ui.graph->getOrientedGraph()) {
            QMessageBox::warning(this, "Unoriented graph",
                                 "Strongly Connected Components algorithm is available only for "
                                 "oriented graphs.");
            return;
        }

        if (!openStepDurationDialog()) {
            return;
        }

        onStartedAlgorithm();

        const auto ctc = new StronglyConnectedComponents(ui.graph);
        connect(ctc, &StronglyConnectedComponents::finished, this, &GraphApp::onFinishedAlgorithm);
        connect(ctc, &StronglyConnectedComponents::aborted, this, &GraphApp::onEndedAlgorithm);

        ctc->setStepDelay(m_stepDelay);
        ctc->showPseudocode();
        ctc->start();
    });

    connect(ui.actionFill_Graph_With_Nodes, &QAction::triggered, [this]() {
        ui.graph->addNode({Node::k_radius, Node::k_radius});
        ui.graph->getNodeManager().setCollisionsCheckEnabled(false);

        auto& nodeManager = ui.graph->getNodeManager();
        const auto sceneSize = ui.graph->getGraphSize().toPoint();

        int lastX = Node::k_radius;
        int lastY = Node::k_radius;

        NodeIndex_t lastIndex = -1;
        while (lastIndex != nodeManager.getNodesCount()) {
            int newX = lastX + Node::k_radius * 2 + 5.;
            if (newX + Node::k_radius >= sceneSize.x()) {
                newX = Node::k_radius;
                lastY += Node::k_radius * 2 + 5.;
            }
            int newY = lastY;

            lastIndex = nodeManager.getNodesCount();
            nodeManager.addNode({newX, newY});

            lastX = newX;
            lastY = newY;
        }

        nodeManager.setCollisionsCheckEnabled(true);
    });
}

void GraphApp::onZoomChanged() {
    ui.zoomScaleText->setText(QString("Graph scale: %1%").arg(ui.graph->getZoomPercentage()));
}

void GraphApp::onStartedAlgorithm() {
    ui.menuBar->setEnabled(false);
    ui.textEdit->setEnabled(false);
    ui.graph->disableEditing();
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
    ui.graph->enableEditing();
}

bool GraphApp::openStepDurationDialog() {
    bool ok = false;
    m_stepDelay = QInputDialog::getInt(nullptr, "Step Delay", "Enter step delay (ms):", 1000, 0,
                                       10000, 50, &ok);

    return ok;
}

bool GraphApp::openPathSourceNodeDialog() {
    bool ok = false;
    m_sourceNodePath =
        QInputDialog::getInt(nullptr, "Source node", "Enter node:", 0, 0,
                             static_cast<int>(ui.graph->getNodesCount() - 1), 1, &ok);

    return ok;
}

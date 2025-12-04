#include <pch.h>

#include "GraphApp.h"

#include "../random/Random.h"

#include "../graph/algorithms/traversals/GenericTraversal.h"
#include "../graph/algorithms/traversals/GenericTotalTraversal.h"
#include "../graph/algorithms/traversals/BreadthTraversal.h"
#include "../graph/algorithms/traversals/DepthTraversal.h"

#include "../graph/algorithms/paths/Path.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent), m_scene(new QGraphicsScene(this)) {
    ui.setupUi(this);

    m_scene->setSceneRect(0, 0, ui.graph->width(), ui.graph->height());
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    ui.graph->setScene(m_scene);

    connect(ui.textEdit, &QTextEdit::textChanged,
            [this]() { ui.graph->onAdjacencyListChanged(ui.textEdit->toPlainText()); });

    connect(ui.graph, &Graph::zoomChanged, this, &GraphApp::onZoomChanged);
    connect(ui.graph, &Graph::endedAlgorithm, this, &GraphApp::onEndedAlgorithm);

    connect(ui.actionComplete_Graph, &QAction::triggered, [this]() {
        QString out;

        const auto nodesCount = ui.graph->getNodesCount();
        for (size_t i = 0; i < nodesCount; ++i) {
            for (size_t j = 0; j < nodesCount; ++j) {
                out += QString("%1 %2\n").arg(i).arg(j);
            }
        }

        ui.textEdit->setText(out);
    });

    connect(ui.actionRandom_Graph, &QAction::triggered, [this]() {
        bool ok = false;
        const auto connectChance =
            QInputDialog::getInt(nullptr, "Chance", "Connection chance:", 20, 0, 100, 1, &ok);
        if (!ok) {
            return;
        }

        QString out;

        const auto nodesCount = ui.graph->getNodesCount();
        for (size_t i = 0; i < nodesCount; ++i) {
            for (size_t j = 0; j < nodesCount; ++j) {
                if (Random::get().getInt(0, 99) < connectChance) {
                    out += QString("%1 %2\n").arg(i).arg(j);
                }
            }
        }

        ui.textEdit->setText(out);
    });

    connect(ui.actionReset_Graph, &QAction::triggered, [this]() {
        ui.graph->resetGraph();
        ui.textEdit->clear();
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
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onEndedAlgorithm);
            connect(traversal, &TraversalBase::finished, traversal, &TraversalBase::deleteLater);

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
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onEndedAlgorithm);
            connect(traversal, &TraversalBase::finished, traversal, &TraversalBase::deleteLater);

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
            connect(path, &Path::finished, this, &GraphApp::onEndedAlgorithm);
            connect(path, &Path::finished, path, &Path::deleteLater);

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
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onEndedAlgorithm);
            connect(traversal, &TraversalBase::finished, traversal, &TraversalBase::deleteLater);

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
            connect(traversal, &TraversalBase::finished, this, &GraphApp::onEndedAlgorithm);
            connect(traversal, &TraversalBase::finished, traversal, &TraversalBase::deleteLater);

            traversal->setStartNode(selectedNode);
            traversal->setStepDelay(m_stepDelay);
            traversal->showPseudocode();
            traversal->start();
        }
    });
}

GraphApp::~GraphApp() {}

void GraphApp::onZoomChanged() {
    ui.zoomScaleText->setText(QString("Graph scale: %1%").arg(ui.graph->getZoomPercentage()));
}

void GraphApp::onStartedAlgorithm() {
    ui.menuBar->setEnabled(false);
    ui.textEdit->setEnabled(false);
    ui.graph->disableEditing();
}

void GraphApp::onEndedAlgorithm() {
    QMessageBox::information(this, "Information",
                             "Algorithm has ended.\nContinuing will reset nodes colors.",
                             QMessageBox::Ok);

    ui.menuBar->setEnabled(true);
    ui.textEdit->setEnabled(true);
    ui.graph->enableEditing();
}

bool GraphApp::openStepDurationDialog() {
    bool ok = false;
    m_stepDelay = QInputDialog::getInt(nullptr, "Step Delay", "Enter step delay (ms):", 500, 0,
                                       5000, 50, &ok);

    return ok;
}

bool GraphApp::openPathSourceNodeDialog() {
    bool ok = false;
    m_sourceNodePath =
        QInputDialog::getInt(nullptr, "Source node", "Enter node:", 0, 0,
                             static_cast<int>(ui.graph->getNodesCount() - 1), 1, &ok);

    return ok;
}

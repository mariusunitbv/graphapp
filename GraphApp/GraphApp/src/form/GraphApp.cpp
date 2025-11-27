#include <pch.h>

#include "GraphApp.h"

#include "../random/Random.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent), m_scene(new QGraphicsScene(this)) {
    ui.setupUi(this);

    m_scene->setSceneRect(0, 0, ui.graph->width(), ui.graph->height());
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
            onStartedAlgorithm();
            ui.graph->genericTraversal(selectedNode);
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
            onStartedAlgorithm();
            ui.graph->genericTotalTraversal(selectedNode);
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
                    "reconstructs the path from an entered node to it's parent.")
                .arg(selectedNode->getIndex()),
            QMessageBox::Ok);

        if (response == QMessageBox::Ok) {
            onStartedAlgorithm();
            ui.graph->path(selectedNode);
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
            onStartedAlgorithm();
            ui.graph->breadthFirstSearch(selectedNode);
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
            onStartedAlgorithm();
            ui.graph->depthFirstSearch(selectedNode);
        }
    });
}

GraphApp::~GraphApp() {}

void GraphApp::onZoomChanged() {
    ui.zoomScaleText->setText(QString("Graph scale: %1%").arg(ui.graph->getZoomPercentage()));
}

void GraphApp::onStartedAlgorithm() {}

void GraphApp::onEndedAlgorithm() {}

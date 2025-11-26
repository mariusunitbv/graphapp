#include <pch.h>

#include "GraphApp.h"

GraphApp::GraphApp(QWidget* parent) : QMainWindow(parent), m_scene(new QGraphicsScene(this)) {
    ui.setupUi(this);

    m_scene->setSceneRect(0, 0, 1280, 720);

    ui.graph->setScene(m_scene);

    connect(ui.textEdit, &QTextEdit::textChanged,
            [this]() { ui.graph->onAdjacencyListChanged(ui.textEdit->toPlainText()); });

    connect(ui.graph, &Graph::zoomChanged, this, &GraphApp::onZoomChanged);

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

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 99);

        const auto nodesCount = ui.graph->getNodesCount();
        for (size_t i = 0; i < nodesCount; ++i) {
            for (size_t j = 0; j < nodesCount; ++j) {
                if (dist(rng) < connectChance) {
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
            setEnabled(false);
            ui.graph->genericTraversal(selectedNode);
            setEnabled(true);
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
            setEnabled(false);
            ui.graph->genericTotalTraversal(selectedNode);
            setEnabled(true);
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
            setEnabled(false);
            ui.graph->path(selectedNode);
            setEnabled(true);
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
            setEnabled(false);
            ui.graph->breadthFirstSearch(selectedNode);
            setEnabled(true);
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
            setEnabled(false);
            ui.graph->depthFirstSearch(selectedNode);
            setEnabled(true);
        }
    });
}

GraphApp::~GraphApp() {}

void GraphApp::onZoomChanged() {
    ui.zoomScaleText->setText(QString("Graph scale: %1%").arg(ui.graph->getZoomPercentage()));
}

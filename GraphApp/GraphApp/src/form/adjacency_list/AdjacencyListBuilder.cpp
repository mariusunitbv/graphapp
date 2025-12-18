#include <pch.h>

#include "AdjacencyListBuilder.h"

AdjacencyListBuilder::AdjacencyListBuilder(Graph* graph, QWidget* parent)
    : QDialog(parent), m_graph(graph) {
    ui.setupUi(this);

    auto& graphManager = m_graph->getGraphManager();

    QString currentAdjacencyList;
    QTextStream stream(&currentAdjacencyList);
    for (size_t i = 0; i < graphManager.getNodesCount(); ++i) {
        graphManager.getGraphStorage()->forEachOutgoingEdge(
            static_cast<NodeIndex_t>(i), [&](NodeIndex_t j, CostType_t cost) {
                if (!graphManager.getAllowLoops() && i == j) {
                    return;
                }

                const bool hasOppositeEdge = graphManager.getGraphStorage()->hasEdge(j, i);
                if (hasOppositeEdge && graphManager.getOrientedGraph()) {
                    const auto oppositeCost = graphManager.getGraphStorage()->getCost(j, i);
                    if (oppositeCost == 0) {
                        stream << j << ' ' << i << '\n';
                    } else {
                        stream << j << ' ' << i << ' ' << oppositeCost << '\n';
                    }
                }

                if (cost == 0) {
                    stream << i << ' ' << j << '\n';
                } else {
                    stream << i << ' ' << j << ' ' << cost << '\n';
                }
            });
    }

    connect(ui.pushButton, &QPushButton::clicked, [this]() { buildAdjacencyFromInput(); });

    ui.plainTextEdit->setPlainText(currentAdjacencyList);
    ui.plainTextEdit->installEventFilter(this);
}

bool AdjacencyListBuilder::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui.plainTextEdit && event->type() == QEvent::KeyPress) {
        auto* e = static_cast<QKeyEvent*>(event);

        if (e->key() == Qt::Key_Return && (e->modifiers() & Qt::ControlModifier)) {
            buildAdjacencyFromInput();
            return true;
        }
    }

    return QDialog::eventFilter(obj, event);
}

void AdjacencyListBuilder::buildAdjacencyFromInput() {
    if (m_graph->buildFromAdjacencyListString(ui.plainTextEdit->toPlainText())) {
        accept();
    } else {
        m_graph->getGraphManager().resetAdjacencyMatrix();
    }
}

#include <pch.h>

#include "AdjacencyListBuilder.h"

AdjacencyListBuilder::AdjacencyListBuilder(QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.pushButton, &QPushButton::clicked, [this]() { accept(); });
}

QString AdjacencyListBuilder::getAdjacencyListText() const {
    return ui.plainTextEdit->toPlainText();
}

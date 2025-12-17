#include <pch.h>

#include "SceneSizeInput.h"

SceneSizeInput::SceneSizeInput(QSize currentSize, QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

    ui.widthBox->setValue(currentSize.width());
    ui.heightBox->setValue(currentSize.height());

    connect(ui.pushButton, &QPushButton::clicked, [this]() { accept(); });
}

QSize SceneSizeInput::getEnteredSize() const {
    return {ui.widthBox->value(), ui.heightBox->value()};
}

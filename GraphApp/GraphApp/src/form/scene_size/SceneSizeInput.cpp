#include <pch.h>

#include "SceneSizeInput.h"

SceneSizeInput::SceneSizeInput(QSize currentSize, QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    ui.widthBox->setValue(currentSize.width());
    ui.heightBox->setValue(currentSize.height());

    connect(ui.pushButton, &QPushButton::clicked, [this]() { accept(); });

    new QShortcut(Qt::Key_M, this, [this]() {
        ui.widthBox->setValue(ui.widthBox->maximum());
        ui.heightBox->setValue(ui.heightBox->maximum());
    });
}

QSize SceneSizeInput::getEnteredSize() const {
    return {ui.widthBox->value(), ui.heightBox->value()};
}

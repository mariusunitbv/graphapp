#include <pch.h>

#include "PbfLoadSettings.h"

PbfLoadSettings::PbfLoadSettings(QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.pushButton, &QPushButton::clicked, [this]() { accept(); });
}

#include <pch.h>

#include "PlaybackSettings.h"

PlaybackSettings::PlaybackSettings(QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.startPlayback, &QPushButton::clicked, [this]() { accept(); });
    connect(ui.runInstantly, &QPushButton::clicked, [this]() {
        m_runInstantly = true;
        accept();
    });

    connect(ui.iterationsPerStep, &QSpinBox::valueChanged,
            [this](int value) { ui.showPseudocode->setEnabled(value == 1); });
}

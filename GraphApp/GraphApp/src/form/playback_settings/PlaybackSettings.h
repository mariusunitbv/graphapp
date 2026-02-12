#pragma once

#include "ui_PlaybackSettings.h"

class PlaybackSettings : public QDialog {
    Q_OBJECT

   public:
    PlaybackSettings(QWidget* parent = nullptr);

    int getStepDelay() const { return ui.stepDelay->value(); }
    int getIterationsPerStep() const { return ui.iterationsPerStep->value(); }

    bool showPseudocode() const {
        return ui.showPseudocode->isChecked() && ui.showPseudocode->isEnabled();
    }
    bool startPaused() const { return ui.startPaused->isChecked(); }

    bool runInstantly() const { return m_runInstantly; }

   private:
    bool m_runInstantly{false};

    Ui_PlaybackSettingsClass ui;
};

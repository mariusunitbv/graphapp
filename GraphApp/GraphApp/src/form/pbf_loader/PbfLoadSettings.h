#pragma once

#include "ui_PbfLoadSettings.h"

class PbfLoadSettings : public QDialog {
    Q_OBJECT

   public:
    PbfLoadSettings(QWidget* parent = nullptr);

    bool parseMotorways() const { return ui.motorway->isChecked(); }
    bool parseMotorwayLinks() const { return ui.motorwaylink->isChecked(); }

    bool parseTrunks() const { return ui.trunk->isChecked(); }
    bool parseTrunkLinks() const { return ui.trunklink->isChecked(); }

    bool parsePrimarys() const { return ui.primary->isChecked(); }
    bool parsePrimaryLinks() const { return ui.primarylink->isChecked(); }

    bool parseSecondarys() const { return ui.secondary->isChecked(); }
    bool parseSecondaryLinks() const { return ui.secondarylink->isChecked(); }

    bool parseTertiarys() const { return ui.tertiary->isChecked(); }
    bool parseTertiaryLinks() const { return ui.tertiarylink->isChecked(); }

    bool parseUnclassifieds() const { return ui.unclassified->isChecked(); }
    bool parseResidentials() const { return ui.residential->isChecked(); }
    bool parseLivingStreets() const { return ui.livingstreet->isChecked(); }
    bool parseServices() const { return ui.service->isChecked(); }
    bool parsePedestrians() const { return ui.pedestrian->isChecked(); }

    bool parseBoundaries() const { return ui.parseBoundaries->isChecked(); }

    int getMergeFactor() const { return ui.mergeFactor->currentIndex(); }

   private:
    Ui_PbfLoadSettingsClass ui;
};

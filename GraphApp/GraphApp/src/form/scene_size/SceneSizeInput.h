#pragma once

#include "ui_SceneSizeInput.h"

class SceneSizeInput : public QDialog {
    Q_OBJECT

   public:
    SceneSizeInput(QSize currentSize, QWidget* parent = nullptr);

    QSize getEnteredSize() const;

   private:
    Ui::SceneSizeInputClass ui;
};

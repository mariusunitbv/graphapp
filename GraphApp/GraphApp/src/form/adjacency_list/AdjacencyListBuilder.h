#pragma once

#include "ui_AdjacencyListBuilder.h"

class AdjacencyListBuilder : public QDialog {
    Q_OBJECT

   public:
    AdjacencyListBuilder(QWidget* parent = nullptr);

    QString getAdjacencyListText() const;

   private:
    Ui::AdjacencyListBuilderClass ui;
};

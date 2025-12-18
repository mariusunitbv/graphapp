#pragma once

#include "ui_LoadingScreen.h"

class LoadingScreen : public QMainWindow {
    Q_OBJECT

   public:
    LoadingScreen(const QString& text, QWidget* parent = nullptr);

    void setText(const QString& text);
    void forceShow();

   private:
    Ui::LoadingScreenClass ui;
};

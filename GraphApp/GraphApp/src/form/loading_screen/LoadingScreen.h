#pragma once

#include "ui_LoadingScreen.h"

class LoadingScreen : public QMainWindow {
    Q_OBJECT

   public:
    LoadingScreen(const QString& text, QWidget* parent = nullptr);

    void setText(const QString& text);
    void forceShow();

   protected:
    void paintEvent(QPaintEvent* event);

   private:
    Ui::LoadingScreenClass ui;
};

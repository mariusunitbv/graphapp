#include <pch.h>

#include "LoadingScreen.h"

LoadingScreen::LoadingScreen(const QString& text, QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);

    setText(text);
}

void LoadingScreen::setText(const QString& text) {
    ui.loadingText->setText(text);
    ui.loadingText->repaint();
}

void LoadingScreen::forceShow() {
    show();

    for (short i = 0; i < 5; ++i) {
        QApplication::processEvents();
    }
}

void LoadingScreen::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    painter.fillRect(rect(), QColor{80, 80, 80});

    Q_UNUSED(event);
}

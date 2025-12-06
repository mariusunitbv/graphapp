#pragma once

#include "ui_PseudocodeForm.h"

class PseudocodeForm : public QMainWindow {
    Q_OBJECT

   public:
    PseudocodeForm(QWidget* parent = nullptr);

    void setHighlightColor(QRgb color);
    void setPseudocodeText(const QString& text);

    void highlight(std::initializer_list<int> lineNumbers);

   private:
    void highlightInternal();

    Ui::PseudocodeFormClass ui;

    QRgb m_highlightColor;
    int m_alpha{0};

    std::vector<int> m_currentlyHighlightedLines;
    QPointer<QVariantAnimation> m_highlightAnimation;
};

#pragma once

#include "ui_PseudocodeForm.h"

class PseudocodeForm : public QMainWindow {
    Q_OBJECT

   public:
    PseudocodeForm(QWidget* parent = nullptr);
    ~PseudocodeForm();

    void setPseudocodeText(const QString& text);

    void highlightLine(int lineNumber);
    void highlightLines(const std::vector<int>& lineNumbers);

   private:
    Ui::PseudocodeFormClass ui;
};

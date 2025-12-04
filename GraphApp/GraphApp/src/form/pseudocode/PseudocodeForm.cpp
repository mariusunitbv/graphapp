#include <pch.h>

#include "PseudocodeForm.h"

PseudocodeForm::PseudocodeForm(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
}

PseudocodeForm::~PseudocodeForm() {}

void PseudocodeForm::setPseudocodeText(const QString& text) {
    ui.plainTextEdit->setPlainText(text);
}

void PseudocodeForm::highlightLine(int lineNumber) {
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::magenta);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCursor cursor(ui.plainTextEdit->document()->findBlockByLineNumber(lineNumber - 1));
    selection.cursor = cursor;
    extraSelections.append(selection);

    ui.plainTextEdit->setExtraSelections(extraSelections);
}

void PseudocodeForm::highlightLines(const std::vector<int>& lineNumbers) {
    QList<QTextEdit::ExtraSelection> extraSelections;

    QColor lineColor = QColor(Qt::magenta);

    for (int lineNumber : lineNumbers) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor cursor(ui.plainTextEdit->document()->findBlockByLineNumber(lineNumber - 1));
        selection.cursor = cursor;
        extraSelections.append(selection);
    }

    ui.plainTextEdit->setExtraSelections(extraSelections);
}

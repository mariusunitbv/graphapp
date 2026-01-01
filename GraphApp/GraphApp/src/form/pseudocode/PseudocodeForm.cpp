#include <pch.h>

#include "PseudocodeForm.h"

PseudocodeForm::PseudocodeForm(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setHighlightColor(qRgb(41, 128, 204));
}

void PseudocodeForm::setHighlightColor(QRgb color) { m_highlightColor = color; }

void PseudocodeForm::setPseudocodeText(const QString& text) {
    ui.plainTextEdit->setPlainText(text);
}

void PseudocodeForm::setHighlightPriority(int priority) { m_priority = priority; }

void PseudocodeForm::highlight(std::initializer_list<int> lineNumbers, int priority) {
    if (isHidden() || priority < m_priority) {
        return;
    }

    m_currentlyHighlightedLines = lineNumbers;

    if (m_highlightAnimation) {
        m_highlightAnimation->stop();
    }

    m_highlightAnimation = new QVariantAnimation(this);
    m_highlightAnimation->setStartValue(1);
    m_highlightAnimation->setEndValue(255);
    m_highlightAnimation->setDuration(k_animationDurationMs);
    m_highlightAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    connect(m_highlightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& v) {
                m_alpha = v.toInt();
                highlightInternal();
            });

    m_highlightAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void PseudocodeForm::highlightInternal() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.reserve(m_currentlyHighlightedLines.size());

    QColor color = QColor::fromRgba(m_highlightColor);
    color.setAlpha(m_alpha);

    for (auto lineNumber : m_currentlyHighlightedLines) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(color);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor cursor(ui.plainTextEdit->document()->findBlockByLineNumber(lineNumber - 1));
        selection.cursor = cursor;
        extraSelections.append(selection);
    }

    ui.plainTextEdit->setExtraSelections(extraSelections);
}

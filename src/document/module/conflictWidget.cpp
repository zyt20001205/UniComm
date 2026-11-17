#include "document/module/conflictWidget.h"

#include <QDir>
#include <QFile>
#include <QShortcut>
#include <QTimer>

#include "globals.h"
#include "core/globalManager.h"
#include "core/themeManager.h"
#include "document/module/scintillaWidget.h"
#include "types/scintillaTypes.h"

// public
ConflictWidget::ConflictWidget(const QJsonObject &documentConfig, const QUrl &documentUrl, QWidget *parent)
    : EditorWidget(documentConfig, documentUrl, parent){
    m_scintillaWidget->viewport()->installEventFilter(this);
}

void ConflictWidget::propertySet(const QVariantHash &objects) {
    m_toolTip = qvariant_cast<QObject *>(objects["mainWindowToolTip"]);
    EditorWidget::propertySet(objects);
    QTimer::singleShot(0, [this] { contentChange(); });
}

bool ConflictWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_scintillaWidget->viewport()) {
        const QPoint globalPos = QCursor::pos();
        const QPoint localPos = m_scintillaWidget->viewport()->mapFromGlobal(globalPos);
        // within margin
        if (localPos.x() < m_scintillaWidget->marginWidthGet()) {
            m_toolTip->setProperty("text", QString());
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const auto position = m_scintillaWidget->cast<Scintilla::Position>(ScintillaWidget::ViewportPoint{localPos});
            // text area click
            if (mouseEvent->button() == Qt::LeftButton) {
                const auto index = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(position);
                const auto line = index.line;
                const int indicators = m_scintillaWidget->indicatorGet(position);
                if (indicators & (1 << ScintillaIndicator::ConflictStart | 1 << ScintillaIndicator::ConflictSeparator | 1 << ScintillaIndicator::ConflictEnd)) {
                    const auto hunk = m_hunk.value(line);
                    if (hunk.size() != 3) return false;
                    const int start = hunk[0];
                    const int separator = hunk[1];
                    const int end = hunk[2];

                    QString text{};
                    if (indicators & 1 << ScintillaIndicator::ConflictStart) {
                        text = m_scintillaWidget->textGet(start + 1, 0, separator - 1, -1);
                    } else if (indicators & 1 << ScintillaIndicator::ConflictSeparator) {
                        text = m_scintillaWidget->textGet(start + 1, 0, separator, 0)
                               + m_scintillaWidget->textGet(separator + 1, 0, end - 1, -1);
                    } else {
                        text = m_scintillaWidget->textGet(separator + 1, 0, end - 1, -1);
                    }
                    m_scintillaWidget->selectionSet(start, 0, end, -1);
                    m_scintillaWidget->textSetSelected(text);
                    contentChange();
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            const auto position = m_scintillaWidget->cast<Scintilla::Position>(ScintillaWidget::ViewportPoint{localPos});
            QString tooltip{};
            if (m_scintillaWidget->indicatorGet(position) & 1 << ScintillaIndicator::ConflictStart) tooltip = tr("Accept Current");
            else if (m_scintillaWidget->indicatorGet(position) & 1 << ScintillaIndicator::ConflictSeparator) tooltip = tr("Accept Both");
            else if (m_scintillaWidget->indicatorGet(position) & 1 << ScintillaIndicator::ConflictEnd) tooltip = tr("Accept Incoming");
            else tooltip = QString();
            if (!tooltip.isEmpty()) {
                m_scintillaWidget->viewport()->setCursor(Qt::PointingHandCursor);
                m_toolTip->setProperty("position", QCursor::pos());
            } else {
                m_scintillaWidget->viewport()->setCursor(Qt::IBeamCursor);
            }
            m_toolTip->setProperty("text", tooltip);
            return false;
        }
    }
    return EditorWidget::eventFilter(watched, event);
}

// protected
void ConflictWidget::indicatorInit() const {
    EditorWidget::indicatorInit();
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::ConflictStart,
        QVariantHash{
            {"style", INDIC_TEXTFORE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->themeGet()->successFore2Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::ConflictSeparator,
        QVariantHash{
            {"style", INDIC_TEXTFORE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->themeGet()->brandBackGet())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::ConflictEnd,
        QVariantHash{
            {"style", INDIC_TEXTFORE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->themeGet()->warningFore2Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
}

void ConflictWidget::markerInit() const {
    EditorWidget::markerInit();
    m_scintillaWidget->markerDefine(
        ScintillaMarker::ConflictCurrent,
        QVariantHash{
            {"symbol", SC_MARK_BACKGROUND},
            {"back", ScintillaWidget::colorGet(g_globalManager->themeGet()->successBack2Get())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::ConflictIncoming,
        QVariantHash{
            {"symbol", SC_MARK_BACKGROUND},
            {"back", ScintillaWidget::colorGet(g_globalManager->themeGet()->warningBack2Get())}
        });
}

void ConflictWidget::contentChange() {
    m_head.clear();
    m_hunk.clear();
    m_scintillaWidget->indicatorClear(ScintillaIndicator::ConflictStart);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::ConflictSeparator);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::ConflictEnd);
    m_scintillaWidget->markerDelete(ScintillaMarker::ConflictCurrent);
    m_scintillaWidget->markerDelete(ScintillaMarker::ConflictIncoming);
    int start = -1;
    int separator = -1;
    int end = -1;
    for (int i = 0; i < m_scintillaWidget->lineCountGet(); ++i) {
        const auto &text = m_scintillaWidget->textGet(i, 0, i, -1);
        if (text.startsWith("<<<<<<<")) start = i;
        else if (text.startsWith("=======")) separator = i;
        else if (text.startsWith(">>>>>>>")) {
            end = i;
            if (start != -1 && separator != -1 && end != -1) {
                m_head.append(start);
                m_hunk.insert(start, QList{start, separator, end});
                m_hunk.insert(separator, QList{start, separator, end});
                m_hunk.insert(end, QList{start, separator, end});
                m_scintillaWidget->indicatorFill(ScintillaIndicator::ConflictStart, start, 0, start, -1);
                m_scintillaWidget->indicatorFill(ScintillaIndicator::ConflictSeparator, separator, 0, separator, -1);
                m_scintillaWidget->indicatorFill(ScintillaIndicator::ConflictEnd, end, 0, end, -1);
                for (int j = start + 1; j < separator; ++j) m_scintillaWidget->markerAdd(ScintillaMarker::ConflictCurrent, j);
                for (int j = separator + 1; j < end; ++j) m_scintillaWidget->markerAdd(ScintillaMarker::ConflictIncoming, j);
            }
            start = -1;
            separator = -1;
        }
    }
    emit statResolve(m_head.size());
}

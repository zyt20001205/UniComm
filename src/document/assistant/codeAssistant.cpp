#include "document/assistant/codeAssistant.h"

#include "globals.h"
#include "document/assistant/completionWidget.h"
#include "document/assistant/dwellWidget.h"
#include "document/assistant/navigationWidget.h"
#include "document/assistant/positionWidget.h"
#include "document/assistant/searchWindow.h"
#include "document/assistant/signatureWidget.h"

// public
CodeAssistant::CodeAssistant(QWidget *parent)
    : QObject(parent),
      m_completionWidget(new CompletionWidget(parent)),
      m_dwellWidget(new DwellWidget(parent)),
      m_navigationWidget(new NavigationWidget(parent)),
      m_positionWidget(new PositionWidget(parent)),
      m_searchWindow(new SearchWindow(parent)),
      m_signatureWidget(new SignatureWidget(parent)) {
    connect(m_completionWidget, &CompletionWidget::addChar, this, &CodeAssistant::addChar);
    connect(m_completionWidget, &CompletionWidget::setIndex, this, &CodeAssistant::setIndex);
    connect(m_completionWidget, &CompletionWidget::setText, this, &CodeAssistant::setText);
    connect(m_completionWidget, &CompletionWidget::showPosition, m_positionWidget, &PositionWidget::positionShow);
    connect(m_dwellWidget, &DwellWidget::textSet, this, &CodeAssistant::setText);
    connect(m_dwellWidget, &DwellWidget::requestCodeAction, this, &CodeAssistant::requestCodeAction);
    connect(m_navigationWidget, &NavigationWidget::setIndex, this, &CodeAssistant::setIndex);
    connect(m_navigationWidget, &NavigationWidget::getText, this, &CodeAssistant::getText);
    connect(m_navigationWidget, &NavigationWidget::insertIndicator, this, &CodeAssistant::insertIndicator);
    connect(m_navigationWidget, &NavigationWidget::recordNavigation, this, &CodeAssistant::recordNavigation);
    connect(m_positionWidget, &PositionWidget::setText, this, &CodeAssistant::setText);
    connect(m_searchWindow, &SearchWindow::setIndex, this, &CodeAssistant::setIndex);
    connect(m_searchWindow, &SearchWindow::addMarker, this, &CodeAssistant::addMarker);
    connect(m_signatureWidget, &SignatureWidget::addChar, this, &CodeAssistant::addChar);
    connect(m_signatureWidget, &SignatureWidget::setTextSelected, this, &CodeAssistant::setTextSelected);
}

void CodeAssistant::propertySet(const QVariantHash &objects) const {
    m_completionWidget->propertySet();
    m_dwellWidget->propertySet(QVariantHash{
        {"documentModuleDwellToolTip", objects["documentModuleDwellToolTip"]},
        {"documentModuleDwellDiagnosticTextArea", objects["documentModuleDwellDiagnosticTextArea"]},
        {"documentModuleDwellHoverTextArea", objects["documentModuleDwellHoverTextArea"]},
        {"documentModuleDwellCodeActionMenu", objects["documentModuleDwellCodeActionMenu"]},
        {"documentModuleDwellSuggestionMenu", objects["documentModuleDwellSuggestionMenu"]}
    });
    m_navigationWidget->propertySet();
    m_positionWidget->propertySet();
    m_searchWindow->propertySet();
    m_signatureWidget->propertySet();
}

void CodeAssistant::fontSet(const QString &family, const int pointSize) const {
    m_completionWidget->fontSet(family, pointSize);
    m_navigationWidget->fontSet(family, pointSize);
    m_signatureWidget->fontSet(family, pointSize);
}

void CodeAssistant::completionShow(const QVariantHash &completionSession, const QJsonArray &items) const {
    m_completionWidget->completionShow(completionSession, items);
}

void CodeAssistant::completionHide() const {
    m_completionWidget->completionHide();
}

void CodeAssistant::diagnosticShow(const QVariantHash &diagnosticSession, const QString &message) const {
    m_dwellWidget->diagnosticShow(diagnosticSession, message);
}

void CodeAssistant::hoverShow(const QVariantHash &hoverSession, const QString &message) const {
    m_dwellWidget->hoverShow(hoverSession, message);
}

void CodeAssistant::codeActionShow(const QUrl &documentUrl, const QJsonArray &result) const {
    m_dwellWidget->codeActionShow(documentUrl, result);
}

void CodeAssistant::dwellHide() const {
    m_dwellWidget->dwellHide();
}

void CodeAssistant::navigationShow(const QVariantHash &navigationSession, const QJsonArray &navigations) const {
    m_navigationWidget->navigationShow(navigationSession, navigations);
}

void CodeAssistant::positionShow(const QVariantMap &positionSession) const {
    m_positionWidget->positionShow(positionSession);
}

void CodeAssistant::signatureShow(const QVariantHash &signatureSession, const QJsonArray &signatures) const {
    m_signatureWidget->signatureShow(signatureSession, signatures);
}

bool CodeAssistant::eventFilter(QObject *watched, QEvent *event) {
    if (g_terminating) return QObject::eventFilter(watched, event);
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            m_searchWindow->open();
            return true;
        }
        switch (keyEvent->key()) {
            // hide keys
            case Qt::Key_Escape:
                m_navigationWidget->navigationHide();
                m_positionWidget->positionHide();
                [[fallthrough]];
            case Qt::Key_Backspace:
            case Qt::Key_Left:
            case Qt::Key_Right: {
                m_completionWidget->completionHide();
                m_signatureWidget->signatureHide();
            }
                return false;
            // selection keys
            case Qt::Key_Up: {
                if (m_completionWidget->isVisible()) {
                    m_completionWidget->completionPrev();
                    return true;
                }
                if (m_navigationWidget->isVisible()) {
                    m_navigationWidget->navigationPrev();
                    return true;
                }
                m_signatureWidget->signatureHide();
                return false;
            }
            case Qt::Key_Down: {
                if (m_completionWidget->isVisible()) {
                    m_completionWidget->completionNext();
                    return true;
                }
                if (m_navigationWidget->isVisible()) {
                    m_navigationWidget->navigationNext();
                    return true;
                }
                m_signatureWidget->signatureHide();
                return false;
            }
            // completion keys
            case Qt::Key_Tab: {
                if (m_completionWidget->isVisible()) {
                    m_completionWidget->completionAccept();
                    return true;
                }
                if (m_signatureWidget->isVisible()) {
                    m_signatureWidget->signatureNext();
                    return true;
                }
                return false;
            }
            // navigation keys
            case Qt::Key_Return:
            case Qt::Key_Enter: {
                if (m_navigationWidget->isVisible()) {
                    m_navigationWidget->navigationAccept();
                    return true;
                }
                return false;
            }
            default:
                return false;
        }
    }
    if (m_positionWidget->isVisible()) {
        if (event->type() == QEvent::MouseButtonPress) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_positionWidget->textReplace();
                m_completionWidget->completionHide();
                m_signatureWidget->signatureHide();
                return true;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

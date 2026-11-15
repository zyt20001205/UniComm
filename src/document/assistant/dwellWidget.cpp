#include "document/assistant/dwellWidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>

#include "globals.h"
#include "core/globalManager.h"
#include "analysis/nuspellModule.h"
#include "util/uniCast.h"

// public
DwellWidget::DwellWidget(QObject *parent)
    : QObject(parent) {
}

void DwellWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModuleDwellToolTip"]);
    m_tooltip->setProperty("dwellWidget", QVariant::fromValue(this));
    m_diagnosticTextArea = qvariant_cast<QObject *>(objects["documentModuleDwellDiagnosticTextArea"]);
    m_hoverTextArea = qvariant_cast<QObject *>(objects["documentModuleDwellHoverTextArea"]);
    m_codeActionMenu = qvariant_cast<QObject *>(objects["documentModuleDwellCodeActionMenu"]);
    m_suggestionMenu = qvariant_cast<QObject *>(objects["documentModuleDwellSuggestionMenu"]);
}

void DwellWidget::diagnosticShow(const QVariantHash &diagnosticSession, const QString &message) {
    m_documentUrl = diagnosticSession["documentUrl"].toUrl();
    m_diagnosticTextArea->setProperty("text", message);
    if (message.isEmpty()) {
        m_diagnosticTextArea->setProperty("visible", false);
    } else {
        m_diagnosticTextArea->setProperty("visible", true);
        const auto position = diagnosticSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
        QMetaObject::invokeMethod(m_tooltip, "open");
    }
}

void DwellWidget::hoverShow(const QVariantHash &hoverSession, const QString &message) const {
    m_hoverTextArea->setProperty("text", message);
    if (message.isEmpty()) {
        m_hoverTextArea->setProperty("visible", false);
    } else {
        m_hoverTextArea->setProperty("visible", true);
        const auto position = hoverSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
        QMetaObject::invokeMethod(m_tooltip, "open");
    }
}

void DwellWidget::codeActionShow(const QUrl &documentUrl, const QJsonArray &result) const {
    QVariantList codeActions{};
    for (const auto &value: result) {
        const QJsonObject action = value.toObject();
        if (action.contains("edit")) {
            codeActions.append(QVariantMap{
                {"title", action["title"]},
                {"edit", action["edit"]}
            });
        }
    }
    m_tooltip->setProperty("codeActions", codeActions);
    QMetaObject::invokeMethod(m_codeActionMenu, "open");
}

void DwellWidget::dwellHide() const {
    QMetaObject::invokeMethod(m_tooltip, "close");
    m_diagnosticTextArea->setProperty("text", "");
    m_hoverTextArea->setProperty("text", "");
}

void DwellWidget::linkClick(const QUrl &customUrl) {
    // qDebug() << customUrl;
    const QString scheme = customUrl.scheme();
    if (scheme == "request.code.action") {
        const QStringList arguments = customUrl.path().split('/');
        m_diagnosticStartLine = arguments[1].toInt();
        m_diagnosticStartCharacter = arguments[2].toInt();
        m_diagnosticEndLine = arguments[3].toInt();
        m_diagnosticEndCharacter = arguments[4].toInt();
        emit requestCodeAction(m_documentUrl, m_diagnosticStartLine, m_diagnosticStartCharacter, m_diagnosticEndLine, m_diagnosticEndCharacter);
    } else if (scheme == "request.spell.suggestion") {
        const QString host = customUrl.host();
        const QStringList arguments = customUrl.path().split('/');
        m_typoStartLine = arguments[1].toInt();
        m_typoStartCharacter = arguments[2].toInt();
        m_typoEndLine = arguments[3].toInt();
        m_typoEndCharacter = arguments[4].toInt();
        const QStringList suggestions = g_globalManager->nuspellModuleGet()->spellSuggestRequest(host);
        m_tooltip->setProperty("suggestions", suggestions);
        QMetaObject::invokeMethod(m_suggestionMenu, "open");
    }
}

void DwellWidget::suggestionAccept(const QString &text) {
    emit textSet(m_documentUrl, text, m_typoStartLine, m_typoStartCharacter, m_typoEndLine, m_typoEndCharacter);
}

void DwellWidget::codeActionAccept(const QJsonObject &codeAction) {
    const auto changes = codeAction["changes"].toObject();
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        const LUrl uri = it.key();
        const auto documentUrl = uni_cast<QUrl>(uri);
        const QJsonArray changeArray = it.value().toArray();
        for (const auto &value: changeArray) {
            const auto change = value.toObject();
            const auto newText = change["newText"].toString();
            const QJsonObject range = change["range"].toObject();
            const QJsonObject start = range["start"].toObject();
            const QJsonObject end = range["end"].toObject();
            const int startLine = start["line"].toInt();
            const int startCharacter = start["character"].toInt();
            const int endLine = end["line"].toInt();
            const int endCharacter = end["character"].toInt();
            emit textSet(documentUrl, newText, startLine, startCharacter, endLine, endCharacter);
        }
    }
}

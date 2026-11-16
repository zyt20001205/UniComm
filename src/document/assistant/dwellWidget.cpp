#include "document/assistant/dwellWidget.h"

#include <QDebug>
#include <QHideEvent>
#include <QJsonArray>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QVBoxLayout>

#include "globals.h"
#include "core/globalManager.h"
#include "analysis/nuspellModule.h"
#include "document/documentModule.h"
#include "document/module/webviewWidget.h"
#include "util/uniCast.h"

// public
DwellWidget::DwellWidget(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus),
      m_surface(new QWidget(this)) {
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
    m_webview = new WebviewWidget(this);
    m_surface->setObjectName("dwellSurface");
    m_surface->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *layout = new QVBoxLayout(this); // NOLINT
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_webview);
    m_webview->setLinkDelegationEnabled(true);
    m_webview->hide();
    connect(m_webview, &WebviewWidget::errorOccurred, this, [](const QString &message) {
        qWarning().noquote() << "Dwell WebView:" << message;
    });
    connect(m_webview, &WebviewWidget::linkClicked, this, &DwellWidget::linkClick, Qt::QueuedConnection);
    qApp->installEventFilter(this);
}

void DwellWidget::propertySet(const QVariantHash &objects) {
    m_codeActionMenu = qvariant_cast<QObject *>(objects["documentModuleDwellCodeActionMenu"]);
    m_suggestionMenu = qvariant_cast<QObject *>(objects["documentModuleDwellSuggestionMenu"]);
    m_codeActionMenu->setProperty("dwellWidget", QVariant::fromValue(this));
    m_suggestionMenu->setProperty("dwellWidget", QVariant::fromValue(this));
    const auto style = objects["documentModuleDwellStyle"].toHash();
    m_background = style["background"].value<QColor>();
    m_foreground = style["foreground"].value<QColor>();
    const auto border = style["border"].value<QColor>();
    m_surface->setStyleSheet(QString("QWidget#dwellSurface { background: %1; border: 1px solid rgba(%2, %3, %4, %5); border-radius: 4px; }")
        .arg(m_background.name()).arg(border.red()).arg(border.green()).arg(border.blue()).arg(border.alpha()));
}

void DwellWidget::diagnosticSet(const QVariantHash &diagnosticSession, const QJsonArray &diagnostics, const QVariantList &typos) {
    ++m_sessionId;
    m_sessionActive = true;
    m_documentUrl = diagnosticSession["documentUrl"].toUrl();
    m_diagnostics = diagnostics;
    m_typos = typos;
    m_position = diagnosticSession["position"].toPoint();
}

void DwellWidget::hoverShow(const QVariantHash &hoverSession, const QString &message) {
    if (!m_sessionActive || hoverSession["documentUrl"].toUrl() != m_documentUrl) return;
    m_position = hoverSession["position"].toPoint();
    QMetaObject::invokeMethod(m_codeActionMenu, "close");
    QMetaObject::invokeMethod(m_suggestionMenu, "close");
    QStringList sections;
    for (qsizetype i = 0; i < m_diagnostics.size(); ++i) {
        const auto diagnostic = m_diagnostics[i].toObject();
        QString severity;
        switch (diagnostic["severity"].toInt()) {
            case 1: severity = tr("Error"); break;
            case 2: severity = tr("Warning"); break;
            case 3: severity = tr("Info"); break;
            case 4: severity = tr("Hint"); break;
            default: severity = tr("Diagnostic"); break;
        }
        const auto url = QString("request.code.action://session-%1/%2").arg(m_sessionId).arg(i);
        sections.append(QString("**%1:** %2\n\n[%3](%4)")
            .arg(severity, diagnostic["message"].toString(), tr("Code Action"), url));
    }
    for (qsizetype i = 0; i < m_typos.size(); ++i) {
        auto word = m_typos[i].toMap()["word"].toString().toHtmlEscaped();
        for (const auto character: {'\\', '`', '*', '_', '[', ']'}) {
            word.replace(QChar(character), QString("\\") + QChar(character));
        }
        const auto url = QString("request.spell.suggestion://session-%1/%2").arg(m_sessionId).arg(i);
        sections.append(QString("**%1:** %2\n\n[%3](%4)")
            .arg(tr("Typo"), tr("In word '%1'").arg(word), tr("Show Suggestions"), url));
    }
    auto markdown = sections.join("\n\n");
    if (!message.isEmpty()) {
        if (!markdown.isEmpty()) markdown += "\n\n---\n\n";
        markdown += message;
    }
    if (markdown.isEmpty()) {
        hide();
        return;
    }
    if (m_renderedText != markdown) {
        auto html = uni_cast<QFullHtmlString>(markdown).value;
        const auto style = QString("<style>html, body { background: %1; color: %2; } body { padding: 0; }</style>\n")
            .arg(m_background.name(), m_foreground.name());
        html.insert(html.indexOf("</head>"), style);
        m_webview->setHtml(html);
        m_renderedText = markdown;
    }
    const auto *screen = QGuiApplication::screenAt(m_position);
    if (!screen) screen = this->screen();
    const auto available = screen->availableGeometry();
    setFixedSize(qMin(600, available.width()), qMin(336, available.height()));
    m_webview->show();
    move(qBound(available.left(), m_position.x() + 10, available.right() - width() + 1),
         qBound(available.top(), m_position.y() + 10, available.bottom() - height() + 1));
    show();
    raise();
}

void DwellWidget::codeActionShow(const QUrl &documentUrl, const QJsonArray &result) {
    if (!isVisible() || documentUrl != m_documentUrl) return;
    m_codeActions.clear();
    for (const auto &value: result) {
        const QJsonObject action = value.toObject();
        if (action.contains("edit")) {
            m_codeActions.append(QVariantMap{
                {"title", action["title"].toString()},
                {"edit", action["edit"].toObject()}
            });
        }
    }
    emit codeActionsChanged();
    m_codeActionMenu->setProperty("position", mapToGlobal(QPoint(width() - 5, -8)));
    QMetaObject::invokeMethod(m_codeActionMenu, "open");
}

void DwellWidget::dwellHide() {
    m_sessionActive = false;
    m_renderedText.clear();
    m_diagnostics = {};
    m_typos.clear();
    hide();
}

void DwellWidget::linkClick(const QUrl &customUrl) {
    const QString scheme = customUrl.scheme();
    const bool diagnosticLink = scheme == "request.code.action" || scheme == "request.spell.suggestion";
    if (!diagnosticLink) {
        if (scheme != "javascript" && scheme != "data") g_globalManager->documentModuleGet()->documentOpen(customUrl);
        return;
    }
    if (!m_sessionActive || customUrl.host() != QString("session-%1").arg(m_sessionId)) return;
    bool validIndex{};
    const int index = customUrl.path().mid(1).toInt(&validIndex);
    if (!validIndex || index < 0) return;
    if (scheme == "request.code.action") {
        if (index >= m_diagnostics.size()) return;
        const auto range = m_diagnostics[index].toObject()["range"].toObject();
        const auto start = range["start"].toObject();
        const auto end = range["end"].toObject();
        emit requestCodeAction(m_documentUrl, start["line"].toInt(), start["character"].toInt(), end["line"].toInt(), end["character"].toInt());
    } else if (scheme == "request.spell.suggestion") {
        if (index >= m_typos.size()) return;
        const auto typo = m_typos[index].toMap();
        m_typoStartLine = m_typoEndLine = typo["line"].toInt();
        m_typoStartCharacter = typo["startCharacter"].toInt();
        m_typoEndCharacter = typo["endCharacter"].toInt();
        const QStringList suggestions = g_globalManager->nuspellModuleGet()->spellSuggestRequest(typo["word"].toString());
        m_suggestions.clear();
        for (const auto &suggestion: suggestions.first(qMin<qsizetype>(5, suggestions.size()))) {
            m_suggestions.append(suggestion);
        }
        emit suggestionsChanged();
        m_suggestionMenu->setProperty("position", mapToGlobal(QPoint(width() - 5, -8)));
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

bool DwellWidget::eventFilter(QObject *watched, QEvent *event) {
    if (m_sessionActive) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
            const auto position = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            const auto insideMenu = [position](QObject *menu) {
                if (!menu || !menu->property("visible").toBool()) return false;
                const auto *content = qvariant_cast<QQuickItem *>(menu->property("contentItem"));
                return content && content->window() && content->window()->geometry().contains(position);
            };
            const bool insidePopup = isVisible() && m_surface->geometry().contains(mapFromGlobal(position));
            if (!insidePopup && !insideMenu(m_codeActionMenu) && !insideMenu(m_suggestionMenu)) dwellHide();
        } else if (event->type() == QEvent::ApplicationDeactivate) {
            dwellHide();
        } else if (watched == parentWidget() && (event->type() == QEvent::Hide || event->type() == QEvent::Close)) {
            dwellHide();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DwellWidget::hideEvent(QHideEvent *event) {
    if (m_codeActionMenu) QMetaObject::invokeMethod(m_codeActionMenu, "close");
    if (m_suggestionMenu) QMetaObject::invokeMethod(m_suggestionMenu, "close");
    m_renderedText.clear();
    m_webview->stop();
    QWidget::hideEvent(event);
}

void DwellWidget::resizeEvent(QResizeEvent *event) {
    m_surface->setGeometry(rect());
    QWidget::resizeEvent(event);
}

#include "document/assistant/completionWidget.h"

#include <QJsonArray>
#include <QQmlComponent>
#include <QtMath>

#include "globals.h"
#include "core/globalManager.h"
#include "port/portModule.h"
#include "types/lspTypes.h"

// public
CompletionWidget::CompletionWidget(QObject *parent)
    : QObject(parent),
      m_placeholderSet({
          "\"__PLACEHOLDER__PORTNAME__\"",
          "\"__PLACEHOLDER__PASSWORD__\""
      }) {
}

void CompletionWidget::propertySet() {
    QQmlComponent component(g_globalManager->engineGet(), QUrl("qrc:/qml/document/assistant/completionWidget.qml"));
    if (auto *root = component.createWithInitialProperties({{"completionWidget", QVariant::fromValue(this)}})) root->setParent(this);
    else qWarning().noquote() << component.errorString();
}

void CompletionWidget::fontSet(const QString &family, const int pointSize) {
    m_font.setFamily(family);
    m_font.setPointSize(pointSize);
    emit fontChanged();
    sizesRefresh();
    if (!m_model.isEmpty()) emit modelChanged();
}

void CompletionWidget::completionShow(const QVariantHash &completionSession, const QJsonArray &items) {
    m_completionSession = completionSession;
    QVariantList model{};
    int completionMode = Full;
    const auto theme = g_globalManager->themeGet();
    const auto general = theme["general"].toObject();
    const int typed = completionSession["typed"].toInt();
    for (const auto &value: items) {
        QJsonObject item = value.toObject();
        const QString label = item["label"].toString();
        const QString insertText = item["insertText"].toString(label);
        if (insertText == "_ENV") {
            completionMode = Simple;
            break;
        }
    }
    QString recordInsertText{};
    for (const auto &value: items) {
        QJsonObject item = value.toObject();
        const int kind = item["kind"].toInt();
        if (completionMode == Simple && kind != LspCompletionKind::EnumMember) continue;
        const QString label = item["label"].toString();
        const QString insertText = item["insertText"].toString(label);
        // placeholder check
        if (m_placeholderSet.contains(insertText)) {
            placeholderExpand(insertText, model);
            continue;
        }
        // duplicate item check
        if (model.isEmpty() || insertText != recordInsertText) {
            recordInsertText = insertText;
            const auto displayText = QString("<span style='color: %1;'>%2</span>%3").arg(
                general["foreDisabled"].toString(),
                insertText.left(typed).toHtmlEscaped(),
                insertText.mid(typed).toHtmlEscaped());
            model.append(QVariantHash{
                {"text", insertText},
                {"displayText", displayText},
                {"kind", kind},
                {"labels", QStringList{label}}
            });
        } else {
            auto item = model.last().toHash();
            auto labels = item["labels"].toStringList();
            labels.append(label);
            item["labels"] = labels;
            model.last() = item;
        }
    }
    if (model.isEmpty()) {
        completionHide();
        return;
    }
    m_model = model;
    sizesRefresh();
    emit modelChanged();
    emit showCompletion(completionSession["position"].toPoint());
}

void CompletionWidget::completionHide() {
    if (!m_model.isEmpty()) {
        m_model.clear();
        sizesRefresh();
        emit modelChanged();
    }
    if (m_visible) emit hideCompletion();
}

void CompletionWidget::textReplace(const int index) {
    if (index < 0 || index >= m_model.size()) return;
    const auto item = m_model[index].toHash();
    const int kind = item["kind"].toInt();
    QString insertText = item["text"].toString();
    if (kind == LspCompletionKind::Method || kind == LspCompletionKind::Function) {
        insertText += "()";
    } else if (kind == LspCompletionKind::Field) {
        insertText += ".";
    } else if (kind == LspCompletionKind::EnumMember) {
        if (insertText == "\"__PLACEHOLDER__GETPOSITION__\"") {
            const QVariantMap gotoSession = {
                {"documentUrl", m_completionSession["documentUrl"].toUrl()},
                {"line", m_completionSession["startLine"].toInt()},
                {"character", m_completionSession["startCharacter"].toInt()}
            };
            emit showPosition(gotoSession);
            completionHide();
            return;
        }
        insertText.replace("\\", "\\\\");
    }
    emit setText(
        m_completionSession["documentUrl"].toUrl(),
        insertText,
        m_completionSession["startLine"].toInt(),
        m_completionSession["startCharacter"].toInt(),
        m_completionSession["endLine"].toInt(),
        m_completionSession["endCharacter"].toInt());
    int cursorPosition = 0;
    if (kind == LspCompletionKind::Method || kind == LspCompletionKind::Function) {
        cursorPosition = m_completionSession["startCharacter"].toInt() + insertText.size() - 1;
    } else {
        cursorPosition = m_completionSession["startCharacter"].toInt() + insertText.size();
    }
    emit setIndex(
        m_completionSession["documentUrl"].toUrl(),
        m_completionSession["startLine"].toInt(),
        cursorPosition);
    if (kind == LspCompletionKind::Method || kind == LspCompletionKind::Function) {
        emit addChar(m_completionSession["documentUrl"].toUrl(), '(');
    } else if (kind == LspCompletionKind::Field) {
        emit addChar(m_completionSession["documentUrl"].toUrl(), '.');
    }
    completionHide();
}

void CompletionWidget::placeholderExpand(const QString &placeholder, QVariantList &model) const {
    if (placeholder == "\"__PLACEHOLDER__PORTNAME__\"") {
        const auto &ports = g_globalManager->portModuleGet()->portsGet();
        const auto theme = g_globalManager->themeGet();
        const auto general = theme["general"].toObject();
        const int typed = m_completionSession["typed"].toInt();
        for (const auto &port: ports) {
            const QString insertText = "\"" + port.toHash().value("name").toString() + "\"";
            const auto displayText = QString("<span style='color: %1;'>%2</span>%3").arg(
                general["foreDisabled"].toString(),
                insertText.left(typed).toHtmlEscaped(),
                insertText.mid(typed).toHtmlEscaped());
            model.append(QVariantHash{
                {"text", insertText},
                {"displayText", displayText},
                {"kind", LspCompletionKind::EnumMember},
                {"labels", QStringList{insertText}}
            });
        }
    } else if (placeholder == "\"__PLACEHOLDER__PASSWORD__\"") {
    }
}

void CompletionWidget::sizesRefresh() {
    const QFontMetricsF metrics(m_font);
    const qreal rowHeight = qMax<qreal>(28, qCeil(metrics.height()) + 4);
    qreal width = 0;
    for (auto &value: m_model) {
        auto &item = get<QVariantHash>(value);
        width = qMax(width, metrics.horizontalAdvance(item.value("text").toString()));
        qreal detailWidth = 0;
        const auto labels = item.value("labels").toStringList();
        for (const auto &label: labels) detailWidth = qMax(detailWidth, metrics.horizontalAdvance(label));
        item.insert("detailWidth", qCeil(detailWidth));
    }
    const QSizeF contentSize(qCeil(width), m_model.size() * rowHeight);
    if (m_rowHeight == rowHeight && m_contentSize == contentSize) return;
    m_rowHeight = rowHeight;
    m_contentSize = contentSize;
    emit sizesChanged();
}

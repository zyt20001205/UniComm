#include "document/assistant/completionWidget.h"

#include <QJsonArray>
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

void CompletionWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModuleCompletionToolTip"]);
    m_listView = qvariant_cast<QObject *>(objects["documentModuleCompletionListView"]);
    m_font = m_tooltip->property("font").value<QFont>();
    sizesRefresh();
    m_tooltip->setProperty("completionWidget", QVariant::fromValue(this));
}

void CompletionWidget::fontSet(const QString &family, const int pointSize) {
    if (!m_tooltip) return;
    auto font = m_tooltip->property("font").value<QFont>();
    font.setFamily(family);
    font.setPointSize(pointSize);
    m_tooltip->setProperty("font", font);
    m_font = font;
    sizesRefresh();
}

bool CompletionWidget::isVisible() const {
    if (!m_tooltip) return false;
    return m_tooltip->property("visible").toBool();
}

void CompletionWidget::completionShow(const QVariantHash &completionSession, const QJsonArray &items) {
    m_completionSession = completionSession;
    QVariantList model{};
    int completionMode = Full;
    const auto theme = g_globalManager->themeGet();
    const auto general = theme["general"].toObject();
    const auto colors = theme["lua"].toObject()["completion"].toObject();
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
            QString iconColor{};
            QUrl iconSource{};
            switch (kind) {
                case LspCompletionKind::Text: {
                    iconColor = colors["text"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolString.svg";
                }
                break;
                case LspCompletionKind::Method:
                case LspCompletionKind::Function: {
                    iconColor = colors[kind == LspCompletionKind::Method ? "method" : "function"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolMethod.svg";
                }
                break;
                case LspCompletionKind::Field: {
                    iconColor = colors["field"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolField.svg";
                }
                break;
                case LspCompletionKind::Variable: {
                    iconColor = colors["variable"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolVariable.svg";
                }
                break;
                case LspCompletionKind::Enum: {
                    iconColor = colors["enum"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolEnum.svg";
                }
                break;
                case LspCompletionKind::Keyword: {
                    iconColor = colors["keyword"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolKeyword.svg";
                }
                break;
                case LspCompletionKind::Snippet: {
                    iconColor = colors["snippet"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolSnippet.svg";
                }
                break;
                case LspCompletionKind::File: {
                    iconColor = colors["file"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolFile.svg";
                }
                break;
                case LspCompletionKind::EnumMember: {
                    iconColor = colors["enumMember"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolEnumMember.svg";
                }
                break;
                case LspCompletionKind::Event: {
                    iconColor = colors["event"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolEvent.svg";
                }
                break;
                case LspCompletionKind::Class: {
                    iconColor = colors["class"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolMisc.svg";
                }
                break;
                case LspCompletionKind::Interface: {
                    iconColor = colors["interface"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolVariable.svg";
                }
                break;
                case LspCompletionKind::Property: {
                    iconColor = colors["property"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolField.svg";
                }
                break;
                case LspCompletionKind::Value: {
                    iconColor = colors["value"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolConstant.svg";
                }
                break;
                case LspCompletionKind::Operator: {
                    iconColor = colors["operator"].toObject()["fore"].toString(general["fore"].toString());
                    iconSource = "qrc:/icon/symbolKeyword.svg";
                }
                break;
                default: {
                    iconColor = general["fore"].toString();
                    iconSource = "qrc:/icon/symbolMisc.svg";
                    emit appendLog(LogLevel::Warning, "contact author:", QString("unsupported completion (kind:%1, text:%2)").arg(QString::number(kind), insertText));
                }
                break;
            }
            const auto displayText = QString("<span style='color: %1;'>%2</span>%3").arg(
                general["foreDisabled"].toString(),
                insertText.left(typed).toHtmlEscaped(),
                insertText.mid(typed).toHtmlEscaped());
            model.append(QVariantHash{
                {"text", insertText},
                {"displayText", displayText},
                {"source", iconSource},
                {"iconColor", iconColor},
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
    m_model = model;
    m_detailModel.clear();
    sizesRefresh();
    emit modelChanged();
    emit detailModelChanged();
    if (!m_model.isEmpty()) {
        const auto position = completionSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
        m_listView->setProperty("currentIndex", 0);
        QMetaObject::invokeMethod(m_tooltip, "open");
    } else {
        completionHide();
    }
}

void CompletionWidget::completionHide() {
    QMetaObject::invokeMethod(m_tooltip, "close");
    if (m_model.isEmpty() && m_detailModel.isEmpty()) return;
    m_model.clear();
    m_detailModel.clear();
    sizesRefresh();
    emit modelChanged();
    emit detailModelChanged();
}

void CompletionWidget::completionPrev() const {
    QMetaObject::invokeMethod(m_listView, "decrementCurrentIndex");
}

void CompletionWidget::completionNext() const {
    QMetaObject::invokeMethod(m_listView, "incrementCurrentIndex");
}

void CompletionWidget::detailReload(const int index) {
    QVariantList model{};
    if (index >= 0 && index < m_model.size()) {
        const auto labels = m_model[index].toHash()["labels"].toStringList();
        for (const auto &label: labels) {
            model.append(QVariantHash{{"text", label}});
        }
    }
    if (m_detailModel == model) return;
    m_detailModel = model;
    sizesRefresh();
    emit detailModelChanged();
}

void CompletionWidget::textReplace() {
    const int index = m_listView->property("currentIndex").toInt();
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
        const auto color = theme["lua"].toObject()["completion"].toObject()["enumMember"].toObject()["fore"].toString(general["fore"].toString());
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
                {"source", QUrl("qrc:/icon/symbolEnumMember.svg")},
                {"iconColor", color},
                {"kind", LspCompletionKind::EnumMember},
                {"labels", QStringList{insertText}}
            });
        }
    } else if (placeholder == "\"__PLACEHOLDER__PASSWORD__\"") {
    }
}

void CompletionWidget::sizesRefresh() {
    const QFontMetricsF metrics(m_font);
    const qreal rowHeight = qMax<qreal>(24, qCeil(metrics.height()) + 4);
    const auto measure = [&metrics, rowHeight](const QVariantList &model) {
        qreal width = 0;
        for (const auto &value: model) {
            width = qMax(width, metrics.horizontalAdvance(value.toHash()["text"].toString()));
        }
        return QSizeF(qCeil(width), model.size() * rowHeight);
    };
    const auto contentSize = measure(m_model);
    const auto detailContentSize = measure(m_detailModel);
    if (m_rowHeight == rowHeight && m_contentSize == contentSize && m_detailContentSize == detailContentSize) return;
    m_rowHeight = rowHeight;
    m_contentSize = contentSize;
    m_detailContentSize = detailContentSize;
    emit sizesChanged();
}

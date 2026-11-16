#include "document/assistant/completionWidget.h"

#include <QJsonArray>
#include <QLabel>
#include <QListView>
#include <QTimer>

#include "globals.h"
#include "core/globalManager.h"
#include "port/portModule.h"

// public
CompletionWidget::CompletionWidget(QObject *parent)
    : QObject(parent),
      m_placeholderSet({
          "\"__PLACEHOLDER__PORTNAME__\"",
          "\"__PLACEHOLDER__PASSWORD__\""
      }),
      m_model(new QStandardItemModel(this)),
      m_detailModel(new QStandardItemModel(this)) {
    auto roles = m_model->roleNames();
    roles[IconColorRole] = "iconColor";
    m_model->setItemRoleNames(roles);
}

void CompletionWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModuleCompletionToolTip"]);
    m_tableView = qvariant_cast<QObject *>(objects["documentModuleCompletionTableView"]);
    m_tooltip->setProperty("completionWidget", QVariant::fromValue(this));
}

void CompletionWidget::fontSet(const QString &family, const int pointSize) const {
    if (!m_tooltip) return;
    auto font = m_tooltip->property("font").value<QFont>();
    font.setFamily(family);
    font.setPointSize(pointSize);
    m_tooltip->setProperty("font", font);
}

bool CompletionWidget::isVisible() const {
    if (!m_tooltip) return false;
    return m_tooltip->property("visible").toBool();
}

void CompletionWidget::completionShow(const QVariantHash &completionSession, const QJsonArray &items) {
    m_tooltip->setProperty("typed", QVariant::fromValue(completionSession["typed"]));
    m_completionSession = completionSession;
    m_model->clear();
    int completionMode = Full;
    const auto *theme = g_globalManager->themeGet();
    const auto colors = theme->configGet()["lua"].toObject()["completion"].toObject();
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
        if (completionMode == Simple && kind != EnumMember) continue;
        const QString label = item["label"].toString();
        const QString insertText = item["insertText"].toString(label);
        // placeholder check
        if (m_placeholderSet.contains(insertText)) {
            placeholderExpand(insertText);
            continue;
        }
        // duplicate item check
        if (insertText != recordInsertText) {
            recordInsertText = insertText;
            auto *standardItem = new QStandardItem(insertText); // NOLINT
            QUrl iconSource{};
            switch (kind) {
                case Text: {
                    standardItem->setData(colors["text"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolString.svg";
                }
                break;
                case Method:
                case Function: {
                    standardItem->setData(colors[kind == Method ? "method" : "function"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolMethod.svg";
                }
                break;
                case Field: {
                    standardItem->setData(colors["field"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolField.svg";
                }
                break;
                case Variable: {
                    standardItem->setData(colors["variable"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolVariable.svg";
                }
                break;
                case Enum: {
                    standardItem->setData(colors["enum"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolEnum.svg";
                }
                break;
                case Keyword: {
                    standardItem->setData(colors["keyword"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolKeyword.svg";
                }
                break;
                case Snippet: {
                    standardItem->setData(colors["snippet"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolSnippet.svg";
                }
                break;
                case File: {
                    standardItem->setData(colors["file"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolFile.svg";
                }
                break;
                case EnumMember: {
                    standardItem->setData(colors["enumMember"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolEnumMember.svg";
                }
                break;
                case Event: {
                    standardItem->setData(colors["event"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolEvent.svg";
                }
                break;
                case Class: {
                    standardItem->setData(colors["class"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolMisc.svg";
                }
                break;
                case Interface: {
                    standardItem->setData(colors["interface"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolVariable.svg";
                }
                break;
                case Property: {
                    standardItem->setData(colors["property"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolField.svg";
                }
                break;
                case Value: {
                    standardItem->setData(colors["value"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolConstant.svg";
                }
                break;
                case Operator: {
                    standardItem->setData(colors["operator"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
                    iconSource = "qrc:/icon/symbolKeyword.svg";
                }
                break;
                default: {
                    standardItem->setData(theme->foreGet(), IconColorRole);
                    iconSource = "qrc:/icon/symbolMisc.svg";
                    emit appendLog(LogLevel::Warning, "contact author:", QString("unsupported completion (kind:%1, text:%2)").arg(QString::number(kind), insertText));
                }
                break;
            }
            standardItem->setData(iconSource, Qt::DecorationRole);
            standardItem->setData(QStringList({label}), Qt::WhatsThisRole);
            standardItem->setData(kind, KindRole);
            m_model->appendRow(standardItem);
        } else {
            auto labelList = m_model->item(m_model->rowCount() - 1, 0)->data(Qt::WhatsThisRole).toStringList();
            labelList.append(label);
            m_model->item(m_model->rowCount() - 1, 0)->setData(labelList, Qt::WhatsThisRole);
        }
    }
    if (m_model->rowCount() > 0) {
        const auto position = completionSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
        m_tableView->setProperty("selectedRow", 0);
        QMetaObject::invokeMethod(m_tooltip, "open");
    } else {
        completionHide();
    }
}

void CompletionWidget::completionHide() const {
    QMetaObject::invokeMethod(m_tooltip, "close");
    m_model->clear();
    m_detailModel->clear();
}

void CompletionWidget::completionPrev() const {
    QMetaObject::invokeMethod(m_tableView, "completionPrev");
}

void CompletionWidget::completionNext() const {
    QMetaObject::invokeMethod(m_tableView, "completionNext");
}

void CompletionWidget::detailReload(const int index) const {
    m_detailModel->clear();
    if (!m_model->item(index, 0)) return;
    const auto labelList = m_model->item(index, 0)->data(Qt::WhatsThisRole).toStringList();
    for (const auto &label: labelList) {
        m_detailModel->appendRow(new QStandardItem(label));
    }
}

void CompletionWidget::textReplace() {
    const int index = m_tableView->property("selectedRow").toInt();
    const int kind = m_model->item(index, 0)->data(KindRole).toInt();
    QString insertText = m_model->item(index, 0)->text();
    if (kind == Method || kind == Function) {
        insertText += "()";
    } else if (kind == Field) {
        insertText += ".";
    } else if (kind == EnumMember) {
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
    if (kind == Method || kind == Function) {
        cursorPosition = m_completionSession["startCharacter"].toInt() + insertText.size() - 1;
    } else {
        cursorPosition = m_completionSession["startCharacter"].toInt() + insertText.size();
    }
    emit setIndex(
        m_completionSession["documentUrl"].toUrl(),
        m_completionSession["startLine"].toInt(),
        cursorPosition);
    if (kind == Method || kind == Function) {
        emit addChar(m_completionSession["documentUrl"].toUrl(), '(');
    } else if (kind == Field) {
        emit addChar(m_completionSession["documentUrl"].toUrl(), '.');
    }
    completionHide();
}

void CompletionWidget::placeholderExpand(const QString &placeholder) const {
    if (placeholder == "\"__PLACEHOLDER__PORTNAME__\"") {
        const auto *model = g_globalManager->portModuleGet()->modelGet();
        for (int i = 0; i < model->rowCount(); ++i) {
            const QString insertText = "\"" + model->item(i, 0)->text() + "\"";
            auto *standardItem = new QStandardItem(insertText); // NOLINT
            standardItem->setData("qrc:/icon/symbolEnumMember.svg", Qt::DecorationRole);
            const auto *theme = g_globalManager->themeGet();
            standardItem->setData(theme->configGet()["lua"].toObject()["completion"].toObject()["enumMember"].toObject()["fore"].toString(theme->foreGet()), IconColorRole);
            standardItem->setData(insertText, Qt::WhatsThisRole);
            standardItem->setData(EnumMember, KindRole);
            m_model->appendRow(standardItem);
        }
    } else if (placeholder == "\"__PLACEHOLDER__PASSWORD__\"") {
    }
}

#include "document/assistant/navigationWidget.h"

#include <QJsonArray>
#include <QQmlComponent>
#include <QtMath>

#include "globals.h"
#include "core/globalManager.h"
#include "document/documentModule.h"
#include "types/scintillaTypes.h"
#include "util/uniCast.h"

// public
NavigationWidget::NavigationWidget(QObject *parent)
    : QObject(parent) {
}

void NavigationWidget::propertySet() {
    QQmlComponent component(g_globalManager->engineGet(), QUrl("qrc:/qml/document/assistant/navigationWidget.qml"));
    if (auto *root = component.createWithInitialProperties({{"navigationWidget", QVariant::fromValue(this)}})) root->setParent(this);
    else qWarning().noquote() << component.errorString();
}

void NavigationWidget::fontSet(const QString &family, const int pointSize) {
    m_font.setFamily(family);
    m_font.setPointSize(pointSize);
    emit fontChanged();
    sizesRefresh();
}

void NavigationWidget::navigationShow(const QVariantHash &navigationSession, const QJsonArray &navigations) {
    auto model = m_model;
    for (const auto &value: navigations) {
        const QJsonObject navigation = value.toObject();
        const LUrl uri = navigation["uri"].toString();
        const auto documentUrl = uni_cast<QUrl>(uri);
        const QJsonObject range = navigation["range"].toObject();
        const QJsonObject start = range["start"].toObject();
        const QJsonObject end = range["end"].toObject();
        const QVariantHash item = {
            {"text", documentUrl.fileName()},
            {"position", QVariantHash{
                {"documentUrl", documentUrl},
                {"startLine", start["line"].toInt()},
                {"startCharacter", start["character"].toInt()},
                {"endLine", end["line"].toInt()},
                {"endCharacter", end["character"].toInt()}
            }}
        };
        if (navigationSession["type"].toString() == "definition") model.prepend(item);
        else model.append(item);
    }
    if (m_model != model) {
        m_model = model;
        sizesRefresh();
        emit modelChanged();
    }
    if (!m_model.isEmpty()) emit showNavigation(navigationSession["position"].toPoint());
    else navigationHide();
}

void NavigationWidget::navigationHide() {
    if (!m_model.isEmpty()) {
        m_model.clear();
        sizesRefresh();
        emit modelChanged();
    }
    if (!m_detailText.isEmpty()) {
        m_detailText.clear();
        emit detailTextChanged();
    }
    if (m_visible) emit hideNavigation();
}

void NavigationWidget::detailReload(const int index) {
    if (index < 0 || index >= m_model.size()) return;
    const auto position = m_model[index].toHash()["position"].toHash();
    const int startLine = position["startLine"].toInt();
    const int endLine = position["endLine"].toInt();
    const int firstLine = qMax(0, startLine - 2);
    auto text = g_globalManager->documentModuleGet()->textGet(position["documentUrl"].toUrl(), firstLine, 0, startLine + 2, -1);
    QString hintText{};
    if (!text.isNull()) {
        const auto general = g_globalManager->themeGet()["general"].toObject();
        text.replace("\r\n", "\n");
        text.replace('\r', '\n');
        auto lines = text.split('\n', Qt::KeepEmptyParts);
        const auto numberWidth = QString::number(firstLine + lines.size()).size();
        for (qsizetype i = 0; i < lines.size(); ++i) {
            const auto lineNumber = firstLine + i;
            const auto line = lines[i];
            QString content{};
            if (lineNumber >= startLine && lineNumber <= endLine) {
                const auto startCharacter = lineNumber == startLine ? position["startCharacter"].toInt() : 0;
                const auto endCharacter = lineNumber == endLine ? position["endCharacter"].toInt() : line.size();
                content = QString("%1<span style='color: %2;'>%3</span>%4").arg(
                    line.left(startCharacter).toHtmlEscaped(),
                    general["warningFore3"].toString(),
                    line.mid(startCharacter, endCharacter - startCharacter).toHtmlEscaped(),
                    line.mid(endCharacter).toHtmlEscaped());
            } else {
                content = line.toHtmlEscaped();
            }
            lines[i] = QString::number(lineNumber + 1).rightJustified(numberWidth) + "|" + content;
        }
        hintText = QString("<span style='white-space: pre;'>%1</span>").arg(lines.join("<br>"));
    }
    if (m_detailText == hintText) return;
    m_detailText = hintText;
    emit detailTextChanged();
}

void NavigationWidget::indicatorInsert(const int index) {
    if (index < 0 || index >= m_model.size()) return;
    const auto position = m_model[index].toHash()["position"].toHash();
    emit recordNavigation(QUrl(), 0, 0);
    emit recordNavigation(
        position["documentUrl"].toUrl(),
        position["startLine"].toInt(),
        position["startCharacter"].toInt());
    emit setIndex(
        position["documentUrl"].toUrl(),
        position["startLine"].toInt(),
        position["startCharacter"].toInt());
    emit insertIndicator(
        position["documentUrl"].toUrl(),
        ScintillaIndicator::Current,
        position["startLine"].toInt(),
        position["startCharacter"].toInt(),
        position["endLine"].toInt(),
        position["endCharacter"].toInt(),
        1000);
    navigationHide();
}

void NavigationWidget::sizesRefresh() {
    const QFontMetricsF metrics(m_font);
    const qreal rowHeight = qMax<qreal>(28, qCeil(metrics.height()) + 4);
    qreal width = 0;
    for (const auto &value: m_model) {
        width = qMax(width, metrics.horizontalAdvance(value.toHash()["text"].toString()));
    }
    const QSizeF contentSize(qCeil(width), m_model.size() * rowHeight);
    if (m_rowHeight == rowHeight && m_contentSize == contentSize) return;
    m_rowHeight = rowHeight;
    m_contentSize = contentSize;
    emit sizesChanged();
}

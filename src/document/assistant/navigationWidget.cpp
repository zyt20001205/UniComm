#include "document/assistant/navigationWidget.h"

#include <QJsonArray>

#include "globals.h"
#include "core/globalManager.h"
#include "document/documentModule.h"
#include "util/uniCast.h"

// public
NavigationWidget::NavigationWidget(QObject *parent)
    : QObject(parent),
      m_navigationModel(new QStandardItemModel(this)) {
}

void NavigationWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModuleNavigationToolTip"]);
    m_tooltip->setProperty("navigationWidget", QVariant::fromValue(this));
    m_tableView = qvariant_cast<QObject *>(objects["documentModuleNavigationTableView"]);
    m_tableView->setProperty("model", QVariant::fromValue(m_navigationModel));
    m_label = qvariant_cast<QObject *>(objects["documentModuleNavigationDetailLabel"]);
}

void NavigationWidget::fontSet(const QString &family, const int pointSize) const {
    if (!m_tooltip) return;
    auto font = m_tooltip->property("font").value<QFont>();
    font.setFamily(family);
    font.setPointSize(pointSize);
    m_tooltip->setProperty("font", font);
}

bool NavigationWidget::isVisible() const {
    if (!m_tooltip) return false;
    return m_tooltip->property("visible").toBool();
}

void NavigationWidget::navigationShow(const QVariantHash &navigationSession, const QJsonArray &navigations) {
    m_navigationSession = navigationSession;
    for (const auto &value: navigations) {
        const QJsonObject navigation = value.toObject();
        const LUrl uri = navigation["uri"].toString();
        const auto documentUrl = uni_cast<QUrl>(uri);
        const QJsonObject range = navigation["range"].toObject();
        const QJsonObject start = range["start"].toObject();
        const QJsonObject end = range["end"].toObject();
        auto *standardItem = new QStandardItem(documentUrl.fileName()); // NOLINT
        const QString type = navigationSession["type"].toString();
        standardItem->setData(QVariantHash({
                                  {"documentUrl", documentUrl},
                                  {"startLine", start["line"].toInt()},
                                  {"startCharacter", start["character"].toInt()},
                                  {"endLine", end["line"].toInt()},
                                  {"endCharacter", end["character"].toInt()}
                              }), Qt::WhatsThisRole);
        if (type == "definition") m_navigationModel->insertRow(0, standardItem);
        else m_navigationModel->appendRow(standardItem);
    }
    if (m_navigationModel->rowCount() > 0) {
        const auto position = navigationSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
        m_tableView->setProperty("selectedRow", 0);
        QMetaObject::invokeMethod(m_tooltip, "open");
    } else {
        navigationHide();
    }
}

void NavigationWidget::navigationHide() const {
    QMetaObject::invokeMethod(m_tooltip, "close");
    m_navigationModel->clear();
}

void NavigationWidget::navigationPrev() const {
    QMetaObject::invokeMethod(m_tableView, "navigationPrev");
}

void NavigationWidget::navigationNext() const {
    QMetaObject::invokeMethod(m_tableView, "navigationNext");
}

void NavigationWidget::detailReload(const int index) {
    m_detailIndex = index;
    const auto *item = m_navigationModel->item(m_detailIndex, 0);
    if (!item) return;
    const auto position = item->data(Qt::WhatsThisRole).toHash();
    const auto hint = g_globalManager->documentModuleGet()->textGet(position["documentUrl"].toUrl(), position["startLine"].toInt(), 0, position["startLine"].toInt(), -1);
    const int startCharacter = position["startCharacter"].toInt();
    const int endCharacter = position["endCharacter"].toInt();
    const auto hintText = QString("<span style='white-space: pre;'>%1<span style='color: orange;'>%2</span>%3</span>").arg(
        hint.left(startCharacter).toHtmlEscaped(),
        hint.mid(startCharacter, endCharacter - startCharacter).toHtmlEscaped(),
        hint.mid(endCharacter).toHtmlEscaped());
    m_label->setProperty("text", hintText);
}

void NavigationWidget::indicatorInsert() {
    const int index = m_tableView->property("selectedRow").toInt();
    const auto position = m_navigationModel->item(index, 0)->data(Qt::WhatsThisRole).toHash();
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

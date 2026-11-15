#include "service/baseService.h"

#include <QJsonDocument>
#include <QTime>

BaseService::BaseService(const QString &name, QObject *parent)
    : QObject(parent),
      m_name(name),
      m_eventModel(new QStandardItemModel(this)) {
    m_eventModel->setHorizontalHeaderLabels({tr("Time"), tr("Event"), tr("Detail")});
}

void BaseService::eventAppend(const QString &event, const QJsonObject &response) const {
    constexpr int eventLimit = 200;
    constexpr qsizetype detailLimit = 512;
    constexpr qsizetype toolTipLimit = 512;
    if (m_eventModel->rowCount() == eventLimit) m_eventModel->removeRow(eventLimit - 1);
    const QJsonDocument document(response);
    const auto detail = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
    auto *detailItem = new QStandardItem(detail.left(detailLimit));
    if (detail.size() <= toolTipLimit)
        detailItem->setToolTip(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
    m_eventModel->insertRow(0, {
        new QStandardItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
        new QStandardItem(event),
        detailItem
    });
}

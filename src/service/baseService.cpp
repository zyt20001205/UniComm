#include "service/baseService.h"

#include <QJsonDocument>
#include <QTime>

namespace {
    constexpr qsizetype detailLimit = 512;
}

BaseService::BaseService(const QString &name, QObject *parent)
    : QObject(parent),
      m_name(name),
      m_createdAt(QDateTime::currentMSecsSinceEpoch()),
      m_eventModel(new QStandardItemModel(this)) {
    m_eventModel->setHorizontalHeaderLabels({tr("Time"), tr("Event"), tr("Detail")});
}

void BaseService::eventAppend(const QString &event, const QJsonObject &response) const {
    const QJsonDocument document(response);
    const auto detail = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
    eventAppend(event, detail, detail.size() <= detailLimit ? QString::fromUtf8(document.toJson(QJsonDocument::Indented)) : QString{});
}

void BaseService::eventAppend(const QString &event, const QString &detail, const QString &toolTip) const {
    constexpr int eventLimit = 200;
    if (m_eventModel->rowCount() == eventLimit) m_eventModel->removeRow(eventLimit - 1);
    auto *detailItem = new QStandardItem(detail.left(detailLimit));
    if (detail.size() <= detailLimit)
        detailItem->setToolTip(toolTip.isNull() ? detail : toolTip);
    m_eventModel->insertRow(0, {
        new QStandardItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
        new QStandardItem(event),
        detailItem
    });
}

#include "service/baseService.h"

#include <QJsonDocument>
#include <QStandardItemModel>
#include <QTime>

BaseService::BaseService(const QString &name, QObject *parent)
    : QObject(parent),
      m_name(name),
      m_eventModel(new QStandardItemModel(this)) {
    m_eventModel->setHorizontalHeaderLabels({tr("Time"), tr("Event"), tr("Detail")});
}

QString BaseService::nameGet() const {
    return m_name;
}

int BaseService::statusGet() const {
    return m_status;
}

QString BaseService::detailGet() const {
    return m_detail;
}

void BaseService::statusSet(const int status) {
    if (m_status == status) return;
    m_status = status;
    emit changeStatus();
}

void BaseService::detailSet(const QString &detail) {
    if (m_detail == detail) return;
    m_detail = detail;
    emit changeDetail();
}

void BaseService::eventAppend(const QString &event, const QJsonObject &response) const {
    constexpr int eventLimit = 200;
    constexpr qsizetype toolTipLimit = 512;
    if (m_eventModel->rowCount() == eventLimit) m_eventModel->removeRow(eventLimit - 1);
    const QJsonDocument document(response);
    const auto detail = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
    auto *detailItem = new QStandardItem(detail);
    if (detail.size() <= toolTipLimit)
        detailItem->setToolTip(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
    m_eventModel->insertRow(0, {
        new QStandardItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
        new QStandardItem(event),
        detailItem
    });
}

QAbstractItemModel *BaseService::eventModelGet() const {
    return m_eventModel;
}

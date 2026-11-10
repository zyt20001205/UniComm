#include "service/baseService.h"

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

void BaseService::eventAppend(const QString &event, const QString &detail) const {
    constexpr int eventLimit = 200;
    if (m_eventModel->rowCount() == eventLimit) m_eventModel->removeRow(eventLimit - 1);
    m_eventModel->insertRow(0, {
        new QStandardItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
        new QStandardItem(event),
        new QStandardItem(detail)
    });
}

QAbstractItemModel *BaseService::eventModelGet() const {
    return m_eventModel;
}

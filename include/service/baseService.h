#ifndef UNICOMM_BASESERVICE_H
#define UNICOMM_BASESERVICE_H

#include <QStandardItemModel>
#include <QObject>

class QJsonObject;

class BaseService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)
    Q_PROPERTY(int status READ statusGet NOTIFY changeStatus)
    Q_PROPERTY(qint64 createdAt MEMBER m_createdAt CONSTANT)
    Q_PROPERTY(QString detail READ detailGet NOTIFY changeDetail)
    Q_PROPERTY(QAbstractItemModel *eventModel READ eventModelGet CONSTANT)

public:
    explicit BaseService(const QString &name, QObject *parent = nullptr);

    [[nodiscard]] int statusGet() const {
        return m_status;
    }

    [[nodiscard]] QString detailGet() const {
        return m_detail;
    }

signals:
    void changeStatus();

    void changeDetail();

protected:
    void statusSet(const int status) {
        if (m_status == status) return;
        m_status = status;
        emit changeStatus();
    }

    void detailSet(const QString &detail) {
        if (m_detail == detail) return;
        m_detail = detail;
        emit changeDetail();
    }

    void eventAppend(const QString &event, const QJsonObject &response) const;

    struct Status {
        enum {
            Stopped = 0,
            Running,
            Error
        };
    };

private:
    [[nodiscard]] QAbstractItemModel *eventModelGet() const {
        return m_eventModel;
    }

    const QString m_name;
    int m_status{Status::Stopped};
    const qint64 m_createdAt;
    QString m_detail{};
    QStandardItemModel *const m_eventModel;
};

#endif //UNICOMM_BASESERVICE_H

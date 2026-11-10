#ifndef UNICOMM_BASESERVICE_H
#define UNICOMM_BASESERVICE_H

#include <QAbstractItemModel>
#include <QObject>

class QJsonObject;
class QStandardItemModel;

class BaseService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ nameGet CONSTANT)
    Q_PROPERTY(int status READ statusGet NOTIFY changeStatus)
    Q_PROPERTY(QString detail READ detailGet NOTIFY changeDetail)
    Q_PROPERTY(QAbstractItemModel *eventModel READ eventModelGet CONSTANT)

public:
    explicit BaseService(const QString &name, QObject *parent = nullptr);

    [[nodiscard]] QString nameGet() const;

    [[nodiscard]] int statusGet() const;

    [[nodiscard]] QString detailGet() const;

signals:
    void changeStatus();

    void changeDetail();

protected:
    void statusSet(int status);

    void detailSet(const QString &detail);

    void eventAppend(const QString &event, const QJsonObject &response) const;

private:
    [[nodiscard]] QAbstractItemModel *eventModelGet() const;

    struct Status {
        enum {
            Stopped = 0,
            Starting = 1,
            Running = 2,
            Error = 3
        };
    };

    const QString m_name;
    int m_status{Status::Stopped};
    QString m_detail{};
    QStandardItemModel *const m_eventModel;
};

#endif //UNICOMM_BASESERVICE_H

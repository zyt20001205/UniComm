#ifndef UNICOMM_NETWORKMANAGER_H
#define UNICOMM_NETWORKMANAGER_H

#include <QNetworkReply>

class NetworkManager final : public QNetworkAccessManager {
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

signals:
    void requestCreated(QNetworkReply *reply);

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *data = nullptr) override;
};

#endif //UNICOMM_NETWORKMANAGER_H

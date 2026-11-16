#include "service/network/networkManager.h"

NetworkManager::NetworkManager(QObject *parent)
    : QNetworkAccessManager(parent) {
}

QNetworkReply *NetworkManager::createRequest(const Operation operation, const QNetworkRequest &request, QIODevice *data) {
    auto *reply = QNetworkAccessManager::createRequest(operation, request, data);
    emit requestCreated(reply);
    return reply;
}

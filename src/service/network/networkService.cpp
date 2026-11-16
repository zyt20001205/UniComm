#include "service/network/networkService.h"

#include <QJsonObject>

NetworkService::NetworkService(QObject *parent)
    : BaseService("Network", parent),
      m_manager(new NetworkManager(this)) {
    detailSet(tr("Active requests: %1").arg(m_activeRequests));
    statusSet(Status::Running);

    connect(m_manager, &NetworkManager::requestCreated, this, [this](QNetworkReply *reply) {
        const auto request = reply->request();
        const auto *origin = request.originatingObject();
        const auto source = origin ? QString::fromLatin1(origin->metaObject()->className()) : tr("Unknown");
        QString method;
        switch (reply->operation()) {
            case QNetworkAccessManager::HeadOperation: method = "HEAD"; break;
            case QNetworkAccessManager::GetOperation: method = "GET"; break;
            case QNetworkAccessManager::PutOperation: method = "PUT"; break;
            case QNetworkAccessManager::PostOperation: method = "POST"; break;
            case QNetworkAccessManager::DeleteOperation: method = "DELETE"; break;
            case QNetworkAccessManager::CustomOperation:
                method = QString::fromLatin1(request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray());
                break;
            default: method = "UNKNOWN"; break;
        }
        QJsonObject detail{{"method", method}, {"url", request.url().toString()}};
        ++m_activeRequests;
        detailSet(tr("Active requests: %1").arg(m_activeRequests));

        connect(reply, &QNetworkReply::finished, this, [this, reply, source, detail]() mutable {
            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            if (statusCode.isValid()) detail.insert("statusCode", statusCode.toInt());
            if (reply->error() != QNetworkReply::NoError) detail.insert("error", reply->errorString());
            --m_activeRequests;
            detailSet(tr("Active requests: %1").arg(m_activeRequests));
            eventAppend(source, detail);
        });
    });
}

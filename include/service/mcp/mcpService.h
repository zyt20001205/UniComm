#ifndef UNICOMM_MCPSERVICE_H
#define UNICOMM_MCPSERVICE_H

#include <QHttpServer>

#include "service/baseService.h"

class McpService final : public BaseService {
public:
    explicit McpService(QObject *parent = nullptr);

private:
    [[nodiscard]] QFuture<QHttpServerResponse> requestHandle(const QHttpServerRequest &request);

    [[nodiscard]] QHttpServerResponse responseResult(const QJsonValue &id, const QString &event, QJsonObject result, bool modern) const;

    [[nodiscard]] static QHttpServerResponse responseError(const QJsonValue &id, int code, const QString &message,
                                                           QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok);

    QHttpServer m_httpServer{};
};

#endif //UNICOMM_MCPSERVICE_H

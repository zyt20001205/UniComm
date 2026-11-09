#ifndef UNICOMM_MCPSERVICE_H
#define UNICOMM_MCPSERVICE_H

#include <QHttpServer>

class McpService final : public QObject {
public:
    explicit McpService(QObject *parent = nullptr);

private:
    [[nodiscard]] QHttpServerResponse requestHandle(const QHttpServerRequest &request) const;

    QHttpServer m_httpServer{};
};

#endif //UNICOMM_MCPSERVICE_H

#ifndef UNICOMM_WEBSOCKETSERVER_H
#define UNICOMM_WEBSOCKETSERVER_H

#include <QJsonObject>

#include "basePort.h"

class RingBuffer;
class QWebSocket;
class QWebSocketServer;

class WebSocketServer final : public BasePort {
    Q_OBJECT

public:
    explicit WebSocketServer(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~WebSocketServer() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void disconnectPeer(const QString &peerIp);

    void clear() override;

    void monitor(bool enabled) override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &peerIp, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek = false) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek = false) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false) override;

signals:
    void connected(const QString &peerIp);

    void disconnected(const QString &peerIp);

    void readyRead(const QString &peerIp);

private:
    [[nodiscard]] bool configureSsl();

    void handleNewConnection();

    void handleServerError();

    void handleConnected(QWebSocket *webSocketServerPeer);

    void handleDisconnected(QWebSocket *webSocketServerPeer);

    void handleMessage(QWebSocket *webSocketServerPeer, const QByteArray &message);

    void handleError(const QWebSocket *webSocketServerPeer);

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData, const QString &peerIp = QString());

    [[nodiscard]] QByteArray handleRead(int length, int timeout, const QString &peerIp, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, const QString &peerIp, bool peek);

    void handleLog(int type, const QByteArray &data, const QWebSocket *webSocketServerPeer);

    QWebSocketServer *m_webSocketServer{};
    QJsonObject m_portConfig{};
    QHash<QString, QWebSocket *> m_peerHash{};
    QHash<QString, RingBuffer *> m_bufferHash{};
};

#endif //UNICOMM_WEBSOCKETSERVER_H

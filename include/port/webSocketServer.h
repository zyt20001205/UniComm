#ifndef UNICOMM_WEBSOCKETSERVER_H
#define UNICOMM_WEBSOCKETSERVER_H

#include <QAbstractSocket>
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

    void clear() override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] bool write(const QByteArray &txData, int messageType, const QString &peerIp, const QString &logFormat, const QString &txSuffix);

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false);

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false);

private:
    void handleNewConnection();

    void handleServerError();

    void handleStateChanged(QWebSocket *webSocketServerPeer, QAbstractSocket::SocketState state);

    void handleMessage(QWebSocket *webSocketServerPeer, const QByteArray &message);

    void handleError(const QWebSocket *webSocketServerPeer);

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData, int messageType, const QString &peerIp);

    [[nodiscard]] QByteArray handleRead(int length, int timeout, const QString &peerIp, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, const QString &peerIp, bool peek);

    void handleLog(int type, const QByteArray &data, const QWebSocket *webSocketServerPeer);

    QWebSocketServer *m_webSocketServer{};
    QJsonObject m_portConfig{};
    QHash<QString, QWebSocket *> m_peerHash{};
    QHash<QString, RingBuffer *> m_bufferHash{};
};

#endif //UNICOMM_WEBSOCKETSERVER_H

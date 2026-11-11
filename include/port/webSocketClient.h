#ifndef UNICOMM_WEBSOCKETCLIENT_H
#define UNICOMM_WEBSOCKETCLIENT_H

#include <QElapsedTimer>
#include <QJsonObject>

#include "basePort.h"
#include "port/module/ringBuffer.h"

class QTimer;
class QWebSocket;

class WebSocketClient final : public BasePort {
    Q_OBJECT

public:
    explicit WebSocketClient(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~WebSocketClient() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    void monitor(bool enabled) override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] bool write(const QByteArray &txData, int messageType, const QString &logFormat, const QString &txSuffix);

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

signals:
    void connected();

    void disconnected();

    void readyRead();

private:
    void handleConnected();

    void handleDisconnected();

    void handleMessage(const QByteArray &message);

    void handleError();

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData, int messageType);

    [[nodiscard]] QByteArray handleRead(int length, int timeout, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, bool peek);

    void handleUpdate();

    void handleLog(int type, const QByteArray &data);

    QWebSocket *m_webSocketClient{};
    QJsonObject m_portConfig{};
    QString m_localHost{};
    int m_localPort{};
    RingBuffer m_buffer;
    QTimer *m_monitorTimer{};
    QElapsedTimer m_activeTimer{};
};

#endif //UNICOMM_WEBSOCKETCLIENT_H

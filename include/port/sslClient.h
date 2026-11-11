#ifndef UNICOMM_SSLCLIENT_H
#define UNICOMM_SSLCLIENT_H

#include <QAbstractSocket>
#include <QJsonObject>

#include "basePort.h"
#include "port/module/ringBuffer.h"

class QSslSocket;

class SslClient final : public BasePort {
    Q_OBJECT

public:
    explicit SslClient(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~SslClient() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

private:
    void handleStateChanged(QAbstractSocket::SocketState state);

    void handleReadyRead();

    void handleError();

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData);

    [[nodiscard]] QByteArray handleRead(int length, int timeout, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, bool peek);

    void handleLog(int type, const QByteArray &data);

    QSslSocket *m_sslClient{};
    QJsonObject m_portConfig{};
    QString m_sslClientLocalHost{};
    int m_sslClientLocalPort{};
    RingBuffer m_buffer;
};

#endif //UNICOMM_SSLCLIENT_H

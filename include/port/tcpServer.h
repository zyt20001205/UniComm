#ifndef UNICOMM_TCPSERVER_H
#define UNICOMM_TCPSERVER_H

#include <QAbstractSocket>
#include <QJsonObject>

#include "basePort.h"

class RingBuffer;
class QTcpServer;
class QTcpSocket;

class TcpServer final : public BasePort {
    Q_OBJECT

public:
    explicit TcpServer(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~TcpServer() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &peerIp, const QString &logFormat, const QString &txSuffix);

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false);

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &peerIp, const QString &logFormat, bool peek = false);

private:
    void handleNewConnection();

    void handleServerError();

    void handleStateChanged(QTcpSocket *tcpServerPeer, QAbstractSocket::SocketState state);

    void handleReadyRead(QTcpSocket *tcpServerPeer);

    void handleError(const QTcpSocket *tcpServerPeer);

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData, const QString &peerIp = QString());

    [[nodiscard]] QByteArray handleRead(int length, int timeout, const QString &peerIp, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, const QString &peerIp, bool peek);

    void handleLog(int type, const QByteArray &data, const QTcpSocket *tcpServerPeer);

    QTcpServer *m_tcpServer{};
    QJsonObject m_portConfig{};
    QHash<QString, QTcpSocket *> m_peerHash{};
    QHash<QString, RingBuffer *> m_bufferHash{};
};

#endif //UNICOMM_TCPSERVER_H

#ifndef UNICOMM_BLUETOOTHLE_H
#define UNICOMM_BLUETOOTHLE_H

#include <QJsonObject>
#include <simpleble/SimpleBLE.h>

#include "basePort.h"
#include "port/module/ringBuffer.h"


class BluetoothLe final : public BasePort {
    Q_OBJECT

public:
    explicit BluetoothLe(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~BluetoothLe() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

signals:
    void disconnected();

    void readyRead();

private:
    [[nodiscard]] bool connectedGet();

    void handleDisconnected();

    void handleNotification(const QByteArray &rxData);

    [[nodiscard]] bool handleWrite(const QByteArray &f_txData);

    [[nodiscard]] QByteArray handleRead(int length, int timeout, bool peek);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout, bool peek);

    void handleLog(int type, const QByteArray &data);

    SimpleBLE::Adapter m_adapter{};
    SimpleBLE::Peripheral m_peripheral{};
    QJsonObject m_portConfig{};
    QString m_peripheralName{};
    RingBuffer m_buffer;
    quint16 m_mtu{};
    bool m_subscribed{};
    bool m_closing{};
};

#endif //UNICOMM_BLUETOOTHLE_H

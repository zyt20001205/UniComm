#ifndef UNICOMM_BLUETOOTHLE_H
#define UNICOMM_BLUETOOTHLE_H

#include <QJsonObject>
#include <QStringList>
#include <memory>
#include <optional>
#include <simpleble/SimpleBLE.h>

#include "basePort.h"
#include "port/module/ringBuffer.h"

class BluetoothLe final : public BasePort {
    Q_OBJECT

public:
    struct Capability {
        enum {
            Read = 0,
            WriteRequest,
            WriteCommand,
            Notify,
            Indicate
        };
    };

    explicit BluetoothLe(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~BluetoothLe() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    [[nodiscard]] QString check(const QString &serviceUuid, const QString &characteristicUuid, const std::optional<QString> &descriptorUuid = std::nullopt,
                                std::optional<int> mode = std::nullopt) const;

    void write(const QByteArray &txData, int mode, const QString &serviceUuid, const QString &characteristicUuid);

    void write(const QByteArray &txData, const QString &serviceUuid, const QString &characteristicUuid, const QString &descriptorUuid);

    [[nodiscard]] QByteArray read(const QString &serviceUuid, const QString &characteristicUuid, const std::optional<QString> &descriptorUuid = std::nullopt);

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &serviceUuid, const QString &characteristicUuid, bool peek = false);

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &serviceUuid, const QString &characteristicUuid, bool peek = false);

    void subscribe(int mode, const QString &serviceUuid, const QString &characteristicUuid);

    void unsubscribe(const QString &serviceUuid, const QString &characteristicUuid);

private:
    bool peripheralGet(const QString &peripheralAddress, std::vector<SimpleBLE::Peripheral> peripherals);

    void handleStateChanged(int state);

    struct Subscription {
        Subscription(const int mode, const int capacity) : mode(mode), buffer(capacity) {
        }

        int mode{};
        bool active{true};
        RingBuffer buffer;
    };

    void handleReadyRead(Subscription *subscription, const QByteArray &data, const QString &serviceUuid, const QString &characteristicUuid);

    void handleLog(int type, const QByteArray &data, const QString &serviceUuid, const QString &characteristicUuid, std::optional<QString> descriptorUuid = std::nullopt);

    struct Characteristic {
        int capabilities{};
        QStringList descriptors{};
        std::shared_ptr<Subscription> subscription{};
    };

    struct State {
        enum {
            Disconnected,
            Connecting,
            Connected,
            Disconnecting
        };
    };

    SimpleBLE::Adapter m_adapter{};
    SimpleBLE::Peripheral m_peripheral{};
    QJsonObject m_portConfig{};
    // service UUID -> characteristic UUID -> Characteristic struct.
    QHash<QString, QHash<QString, Characteristic> > m_services{};
    quint16 m_mtu{};
    int m_state{State::Disconnected};
};

#endif //UNICOMM_BLUETOOTHLE_H

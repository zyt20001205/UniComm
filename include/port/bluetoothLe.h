#ifndef UNICOMM_BLUETOOTHLE_H
#define UNICOMM_BLUETOOTHLE_H

#include <QHash>
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

private:
    bool peripheralGet(const QString &peripheralAddress, std::vector<SimpleBLE::Peripheral> peripherals);

    void handleStateChanged(int state);

    struct State {
        enum {
            Disconnected,
            Connecting,
            Connected,
            Disconnecting
        };
    };

    struct Capability {
        enum {
            None = 0,
            Read = 1 << 0,
            WriteRequest = 1 << 1,
            WriteCommand = 1 << 2,
            Notify = 1 << 3,
            Indicate = 1 << 4
        };
    };

    SimpleBLE::Adapter m_adapter{};
    SimpleBLE::Peripheral m_peripheral{};
    QJsonObject m_portConfig{};
    // service UUID -> characteristic UUID -> capability flags.
    QHash<QString, QHash<QString, int>> m_services{};
    RingBuffer m_buffer;
    quint16 m_mtu{};
    int m_state{State::Disconnected};
};

#endif //UNICOMM_BLUETOOTHLE_H

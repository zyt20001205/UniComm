#include "port/bluetoothLe.h"

#include <QEventLoop>
#include <QScopeGuard>
#include <QTimer>
#include <QVariantList>

#include <stdexcept>

#include "globals.h"

// public
BluetoothLe::BluetoothLe(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

BluetoothLe::~BluetoothLe() {
    close();
}

int BluetoothLe::type() {
    return PortType::BluetoothLe;
}

QJsonObject BluetoothLe::config() {
    return m_portConfig;
}

QVariantHash BluetoothLe::info() {
    QVariantHash services{};
    for (auto service = m_services.constBegin(); service != m_services.constEnd(); ++service) {
        QVariantHash characteristics{};
        for (auto characteristic = service.value().constBegin(); characteristic != service.value().constEnd(); ++characteristic) {
            QVariantList capabilities{};
            if (characteristic.value() & Capability::Read) capabilities.append(QStringLiteral("read"));
            if (characteristic.value() & Capability::WriteRequest) capabilities.append(QStringLiteral("write_request"));
            if (characteristic.value() & Capability::WriteCommand) capabilities.append(QStringLiteral("write_command"));
            if (characteristic.value() & Capability::Notify) capabilities.append(QStringLiteral("notify"));
            if (characteristic.value() & Capability::Indicate) capabilities.append(QStringLiteral("indicate"));
            characteristics.insert(characteristic.key(), capabilities);
        }
        services.insert(service.key(), characteristics);
    }
    QString status{};
    switch (m_state) {
        case State::Disconnected: status = "disconnected";
            break;
        case State::Connecting: status = "connecting";
            break;
        case State::Connected: status = "connected";
            break;
        case State::Disconnecting: status = "disconnecting";
            break;
        default: status = "unknown";
    }
    return {
        {"status", status},
        {"adapter", m_portConfig["adapterAddress"].toString()},
        {"peripheral", m_portConfig["peripheralAddress"].toString()},
        {"services", services},
        {"mtu", QString::number(m_mtu)},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool BluetoothLe::open() {
    if (m_state != State::Disconnected) return m_state == State::Connected;

    close();
    m_buffer.clear();
    const auto portName = m_portConfig["portName"].toString();
    const auto adapterAddress = m_portConfig["adapterAddress"].toString();
    const auto peripheralAddress = m_portConfig["peripheralAddress"].toString();
    handleStateChanged(State::Connecting);

    try {
        for (auto adapter: SimpleBLE::Adapter::get_adapters()) {
            if (QString::fromStdString(adapter.address()).compare(adapterAddress, Qt::CaseInsensitive) != 0) continue;
            m_adapter = adapter;
            break;
        }
        if (!m_adapter.initialized()) throw std::runtime_error("adapter not found");
        if (!m_adapter.is_powered()) throw std::runtime_error("adapter is not powered");

        if (!peripheralGet(peripheralAddress, m_adapter.scan_get_results())) {
            // these optional queries are not supported by every backend.
            try {
                if (!peripheralGet(peripheralAddress, m_adapter.get_paired_peripherals())) peripheralGet(peripheralAddress, m_adapter.get_connected_peripherals());
            } catch (...) {
            }
        }
        if (!m_peripheral.initialized()) {
            auto adapter = m_adapter;
            QEventLoop eventLoop;
            const auto cleanup = qScopeGuard([this, &adapter] {
                adapter.set_callback_on_scan_found(nullptr);
                adapter.set_callback_on_scan_updated(nullptr);
                try {
                    if (adapter.scan_is_active()) adapter.scan_stop();
                } catch (const std::exception &exception) {
                    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("scan stop failed: %1").arg(exception.what()));
                } catch (...) {
                    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "scan stop failed");
                }
            });
            const auto discovered = [this, &eventLoop, &peripheralAddress](SimpleBLE::Peripheral peripheral) {
                if (QString::fromStdString(peripheral.address()).compare(peripheralAddress, Qt::CaseInsensitive) != 0) return;
                QMetaObject::invokeMethod(&eventLoop, [this, &eventLoop, peripheral] {
                    if (m_state != State::Connecting || m_peripheral.initialized()) return;
                    m_peripheral = peripheral;
                    eventLoop.quit();
                }, Qt::QueuedConnection);
            };
            connect(this, &BasePort::refreshPort, &eventLoop, [this, &eventLoop] {
                if (m_state != State::Connecting) eventLoop.quit();
            });
            adapter.set_callback_on_scan_found(discovered);
            adapter.set_callback_on_scan_updated(discovered);
            QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
            adapter.scan_start();
            eventLoop.exec();
        }
        if (m_state != State::Connecting) return false;
        if (!m_peripheral.initialized()) throw std::runtime_error("peripheral not found");

        m_peripheral.set_callback_on_connected([this] {
            QMetaObject::invokeMethod(this, [this] {
                try {
                    if (m_peripheral.initialized() && m_peripheral.is_connected()) handleStateChanged(State::Connected);
                } catch (...) {
                }
            });
        });
        m_peripheral.set_callback_on_disconnected([this] {
            QMetaObject::invokeMethod(this, [this] {
                try {
                    if (m_peripheral.initialized() && !m_peripheral.is_connected()) handleStateChanged(State::Disconnected);
                } catch (...) {
                }
            }, Qt::QueuedConnection);
        });
        if (!m_peripheral.is_connected()) m_peripheral.connect();
        m_mtu = m_peripheral.mtu();
        for (auto service: m_peripheral.services()) {
            auto &characteristics = m_services[QString::fromStdString(service.uuid()).toLower()];
            for (auto characteristic: service.characteristics()) {
                int capabilities = Capability::None;
                if (characteristic.can_read()) capabilities |= Capability::Read;
                if (characteristic.can_write_request()) capabilities |= Capability::WriteRequest;
                if (characteristic.can_write_command()) capabilities |= Capability::WriteCommand;
                if (characteristic.can_notify()) capabilities |= Capability::Notify;
                if (characteristic.can_indicate()) capabilities |= Capability::Indicate;
                characteristics.insert(QString::fromStdString(characteristic.uuid()).toLower(), capabilities);
            }
        }

        handleStateChanged(State::Connected);
        return true;
    } catch (const std::exception &exception) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(portName), QString("open failed: %1").arg(exception.what()));
        close();
        return false;
    }
}

void BluetoothLe::close() {
    if (m_peripheral.initialized()) {
        if (m_state == State::Connected) handleStateChanged(State::Disconnecting);
        try {
            m_peripheral.set_callback_on_connected(nullptr);
            m_peripheral.set_callback_on_disconnected(nullptr);
            m_peripheral.disconnect();
        } catch (const std::exception &exception) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), exception.what());
        }
    }
    m_peripheral = {};
    m_adapter = {};
    handleStateChanged(State::Disconnected);
}

void BluetoothLe::clear() {
    m_buffer.clear();
}

// private
bool BluetoothLe::peripheralGet(const QString &peripheralAddress, std::vector<SimpleBLE::Peripheral> peripherals) {
    for (auto &peripheral: peripherals) {
        if (QString::fromStdString(peripheral.address()).compare(peripheralAddress, Qt::CaseInsensitive) == 0) {
            m_peripheral = peripheral;
            return true;
        }
    }
    return false;
}


void BluetoothLe::handleStateChanged(const int state) {
    if (m_state == state) return;
    m_state = state;
    const auto peripheralAddress = m_portConfig["peripheralAddress"].toString();
    QString message{};
    switch (state) {
        case State::Disconnected:
            m_mtu = 0;
            m_services.clear();
            message = QString("disconnected from %1").arg(peripheralAddress);
            break;
        case State::Connecting:
            message = QString("connecting to %1").arg(peripheralAddress);
            break;
        case State::Connected:
            message = QString("connected to %1").arg(peripheralAddress);
            break;
        case State::Disconnecting:
            message = QString("disconnecting from %1").arg(peripheralAddress);
            break;
        default: break;
    }
    emit refreshPort(m_portConfig["portName"].toString(), state == State::Connected);
    if (!message.isEmpty()) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

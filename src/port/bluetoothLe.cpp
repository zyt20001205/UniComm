#include "port/bluetoothLe.h"

#include <QDeadlineTimer>
#include <QPointer>
#include <QScopeGuard>
#include <QTimer>

#include <stdexcept>
#include <utility>

#include "globals.h"
#include "util/uniCast.h"

// public
BluetoothLe::BluetoothLe(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig) {
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
    QVariantHash subscriptions{};
    qsizetype bufferUsed{};
    for (auto service = m_services.constBegin(); service != m_services.constEnd(); ++service) {
        QVariantHash characteristics{};
        QVariantHash subscribed{};
        for (auto characteristic = service.value().constBegin(); characteristic != service.value().constEnd(); ++characteristic) {
            QVariantList capabilities{};
            if (characteristic.value().capabilities & (1u << Capability::Read)) capabilities.append(QStringLiteral("read"));
            if (characteristic.value().capabilities & (1u << Capability::WriteRequest)) capabilities.append(QStringLiteral("write_request"));
            if (characteristic.value().capabilities & (1u << Capability::WriteCommand)) capabilities.append(QStringLiteral("write_command"));
            if (characteristic.value().capabilities & (1u << Capability::Notify)) capabilities.append(QStringLiteral("notify"));
            if (characteristic.value().capabilities & (1u << Capability::Indicate)) capabilities.append(QStringLiteral("indicate"));
            QVariantList descriptors{};
            for (const auto &descriptor: characteristic.value().descriptors) descriptors.append(descriptor);
            characteristics.insert(characteristic.key(), QVariantHash{
                {"capabilities", capabilities},
                {"descriptors", descriptors}
            });
            if (const auto &subscription = characteristic.value().subscription) {
                const auto used = subscription->buffer.used();
                bufferUsed += used;
                subscribed.insert(characteristic.key(), QVariantHash{
                                      {"mode", subscription->mode},
                                      {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
                                      {"bufferUsed", QString::number(used)}
                                  });
            }
        }
        services.insert(service.key(), characteristics);
        if (!subscribed.isEmpty()) subscriptions.insert(service.key(), subscribed);
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
        {"subscriptions", subscriptions},
        {"mtu", QString::number(m_mtu)},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(bufferUsed)}
    };
}

bool BluetoothLe::open() {
    if (m_state != State::Disconnected) return m_state == State::Connected;

    close();
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
                int capabilities{};
                if (characteristic.can_read()) capabilities |= 1u << Capability::Read;
                if (characteristic.can_write_request()) capabilities |= 1u << Capability::WriteRequest;
                if (characteristic.can_write_command()) capabilities |= 1u << Capability::WriteCommand;
                if (characteristic.can_notify()) capabilities |= 1u << Capability::Notify;
                if (characteristic.can_indicate()) capabilities |= 1u << Capability::Indicate;
                QStringList descriptors{};
                for (auto descriptor: characteristic.descriptors()) descriptors.append(QString::fromStdString(descriptor.uuid()).toLower());
                characteristics.insert(QString::fromStdString(characteristic.uuid()).toLower(), Characteristic{capabilities, descriptors});
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
        for (auto service = m_services.begin(); service != m_services.end(); ++service) {
            for (auto characteristic = service.value().begin(); characteristic != service.value().end(); ++characteristic) {
                try {
                    unsubscribe(service.key(), characteristic.key());
                } catch (const std::exception &exception) {
                    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("unsubscribe failed: %1").arg(exception.what()));
                } catch (...) {
                    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "unsubscribe failed");
                }
            }
        }
        try {
            m_peripheral.set_callback_on_connected(nullptr);
            m_peripheral.set_callback_on_disconnected(nullptr);
            m_peripheral.disconnect();
        } catch (const std::exception &exception) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), exception.what());
        } catch (...) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "disconnect failed");
        }
    }
    m_peripheral = {};
    m_adapter = {};
    handleStateChanged(State::Disconnected);
}

void BluetoothLe::clear() {
    for (auto &service: m_services) {
        for (auto &characteristic: service) {
            if (characteristic.subscription) characteristic.subscription->buffer.clear();
        }
    }
}

QString BluetoothLe::check(const QString &serviceUuid, const QString &characteristicUuid, const int mode) const {
    if (mode < -1 || mode > Capability::Indicate) return "invalid BLE mode";
    if (m_state != State::Connected) return "not connected";
    const auto service = m_services.constFind(serviceUuid);
    if (service == m_services.constEnd()) return "service not found";
    const auto characteristic = service.value().constFind(characteristicUuid);
    if (characteristic == service.value().constEnd()) return "characteristic not found in service";
    if (mode == -1) return {};
    if (characteristic.value().capabilities & (1u << mode)) {
        const auto &subscription = characteristic.value().subscription;
        if ((mode == Capability::Notify || mode == Capability::Indicate) && subscription && subscription->mode != mode)
            return "characteristic already subscribed with a different mode; unsubscribe first";
        return {};
    }
    switch (mode) {
        case Capability::Read: return "characteristic does not support read";
        case Capability::WriteRequest: return "characteristic does not support write_request";
        case Capability::WriteCommand: return "characteristic does not support write_command";
        case Capability::Notify: return "characteristic does not support notify";
        case Capability::Indicate: return "characteristic does not support indicate";
        default: return "characteristic does not support requested capability";
    }
}

void BluetoothLe::write(const QByteArray &txData, const int mode, const QString &serviceUuid, const QString &characteristicUuid) {
    const auto service = serviceUuid.toStdString();
    const auto characteristic = characteristicUuid.toStdString();
    const SimpleBLE::ByteArray data(reinterpret_cast<const uint8_t *>(txData.constData()), static_cast<size_t>(txData.size()));
    if (mode == Capability::WriteRequest) m_peripheral.write_request(service, characteristic, data);
    else m_peripheral.write_command(service, characteristic, data);
    handleLog(LogLevel::Transmit, txData, serviceUuid, characteristicUuid);
}

QByteArray BluetoothLe::read(const QString &serviceUuid, const QString &characteristicUuid) {
    const auto data = m_peripheral.read(serviceUuid.toStdString(), characteristicUuid.toStdString());
    const QByteArray rxData(reinterpret_cast<const char *>(data.data()), static_cast<qsizetype>(data.size()));
    handleLog(LogLevel::Receive, rxData, serviceUuid, characteristicUuid);
    return rxData;
}

QByteArray BluetoothLe::read(const int length, const int timeout, const QString &serviceUuid, const QString &characteristicUuid, const bool peek) {
    const auto subscription = m_services.value(serviceUuid).value(characteristicUuid).subscription;
    if (!subscription || length < 0) return {};
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    QEventLoop eventLoop;
    while (subscription->buffer.used() < minimum) {
        if (!subscription->active || m_state != State::Connected || deadline.hasExpired()) break;
        const auto interval = deadline.isForever() ? 10 : static_cast<int>(qMin<qint64>(10, deadline.remainingTime()));
        QTimer::singleShot(interval, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
    }
    if (!subscription->active || m_state != State::Connected) return {};
    return subscription->buffer.read(length, peek);
}

QByteArray BluetoothLe::readUntil(const QByteArray &text, const int timeout, const QString &serviceUuid, const QString &characteristicUuid, const bool peek) {
    const auto subscription = m_services.value(serviceUuid).value(characteristicUuid).subscription;
    if (!subscription || text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QEventLoop eventLoop;
    QByteArray data = subscription->buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (!subscription->active || m_state != State::Connected || deadline.hasExpired()) break;
        const auto interval = deadline.isForever() ? 10 : static_cast<int>(qMin<qint64>(10, deadline.remainingTime()));
        QTimer::singleShot(interval, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
        if (!subscription->active || m_state != State::Connected) return {};
        data = subscription->buffer.readUntil(text, peek);
    }
    return data;
}

void BluetoothLe::subscribe(const int mode, const QString &serviceUuid, const QString &characteristicUuid) {
    auto &characteristic = m_services[serviceUuid][characteristicUuid];
    if (characteristic.subscription) return;
    const auto subscription = std::make_shared<Subscription>(mode, m_portConfig["bufferSize"].toInt());
    characteristic.subscription = subscription;
    const auto callback = [port = QPointer(this), weak = std::weak_ptr(subscription), serviceUuid, characteristicUuid](SimpleBLE::ByteArray data) {
        if (!port) return;
        const QByteArray rxData(reinterpret_cast<const char *>(data.data()), static_cast<qsizetype>(data.size()));
        QMetaObject::invokeMethod(port.data(), [port, weak, serviceUuid, characteristicUuid, rxData] {
            if (const auto subscription = weak.lock()) port->handleReadyRead(subscription.get(), rxData, serviceUuid, characteristicUuid);
        }, Qt::QueuedConnection);
    };
    try {
        if (mode == Capability::Notify) m_peripheral.notify(serviceUuid.toStdString(), characteristicUuid.toStdString(), callback);
        else m_peripheral.indicate(serviceUuid.toStdString(), characteristicUuid.toStdString(), callback);
    } catch (...) {
        subscription->active = false;
        characteristic.subscription.reset();
        try {
            m_peripheral.unsubscribe(serviceUuid.toStdString(), characteristicUuid.toStdString());
        } catch (...) {
        }
        throw;
    }
}

void BluetoothLe::unsubscribe(const QString &serviceUuid, const QString &characteristicUuid) {
    auto &characteristic = m_services.find(serviceUuid).value().find(characteristicUuid).value();
    const auto subscription = std::exchange(characteristic.subscription, {});
    if (!subscription) return;
    subscription->active = false;
    m_peripheral.unsubscribe(serviceUuid.toStdString(), characteristicUuid.toStdString());
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
            for (auto &service: m_services) {
                for (auto &characteristic: service) {
                    if (characteristic.subscription) characteristic.subscription->active = false;
                }
            }
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

void BluetoothLe::handleReadyRead(Subscription *subscription, const QByteArray &data, const QString &serviceUuid, const QString &characteristicUuid) {
    if (m_state != State::Connected || !subscription->active) return;
    if (subscription->buffer.write(data) != data.size()) {
        const auto source = QString("[%1/%2/%3]").arg(m_portConfig["portName"].toString(), uni_cast<QBluetoothUuidShort>(serviceUuid).value, uni_cast<QBluetoothUuidShort>(characteristicUuid).value);
        emit appendLog(LogLevel::Error, source, "buffer overflow");
        try {
            unsubscribe(serviceUuid, characteristicUuid);
        } catch (const std::exception &exception) {
            emit appendLog(LogLevel::Error, source, QString("unsubscribe failed: %1").arg(exception.what()));
        } catch (...) {
            emit appendLog(LogLevel::Error, source, "unsubscribe failed");
        }
        return;
    }
    handleLog(LogLevel::Receive, data, serviceUuid, characteristicUuid);
}

void BluetoothLe::handleLog(const int type, const QByteArray &data, const QString &serviceUuid, const QString &characteristicUuid) {
    QString message{};
    const auto format = m_portConfig["logFormat"].toString();
    if (format == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (format == "hex") message = data.toHex(' ').toUpper();
    else if (format == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    const auto endpoint = QString("%1/%2/%3").arg(m_portConfig["peripheralAddress"].toString(), uni_cast<QBluetoothUuidShort>(serviceUuid).value, uni_cast<QBluetoothUuidShort>(characteristicUuid).value);
    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1 -&gt; %2]").arg(m_portConfig["adapterAddress"].toString(), endpoint), message);
    } else {
        emit appendLog(type, QString("[%1 &lt;- %2]").arg(m_portConfig["adapterAddress"].toString(), endpoint), message);
    }
}

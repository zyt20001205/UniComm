#include "api/port.h"

#include <sol/state_view.hpp>
#include <sol/table_core.hpp>

#include <exception>
#include <utility>

#include "globals.h"
#include "port/basePort.h"
#include "port/bluetoothLe.h"
#include "port/portModule.h"
#include "port/sslServer.h"
#include "port/tcpServer.h"
#include "port/vision.h"
#include "port/webSocketClient.h"
#include "port/webSocketServer.h"
#include "util/uniCast.h"

Port::Port(QString portName, QObject *parent)
    : QObject(parent),
      m_portName(std::move(portName)),
      m_port(g_port->m_portHash.value(m_portName)) {
}

sol::table Port::list(const sol::this_state ts) {
    QSet<QString> portSet{};

    QMetaObject::invokeMethod(g_port, [&portSet] {
        portSet = g_port->portList();
    }, Qt::BlockingQueuedConnection);
    return uni_cast<sol::table>(ts, portSet);
}

Port *Port::create(const sol::table &config, QObject *parent) {
    auto portConfig = uni_cast<QJsonObject>(config);
    QString error{};
    QMetaObject::invokeMethod(g_port, [&error, &portConfig] {
        error = g_port->portInsert(-1, portConfig);
    }, Qt::BlockingQueuedConnection);
    if (!error.isEmpty()) throw sol::error(error.toStdString());
    return new Port(portConfig["portName"].toString(), parent); // NOLINT
}

Port *Port::get(const std::string &portName, QObject *parent) {
    const auto name = QString::fromStdString(portName);
    if (!g_port->m_portHash.contains(name)) throw sol::error(portName + " does not exist");
    return new Port(name, parent); // NOLINT
}

void Port::remove(const std::string &portName) {
    QString error{};
    QMetaObject::invokeMethod(g_port, [&error, &portName] {
        error = g_port->portRemove(QString::fromStdString(portName));
    }, Qt::BlockingQueuedConnection);
    if (!error.isEmpty()) throw sol::error(error.toStdString());
}

sol::object Port::info(const sol::this_state ts) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    QVariantHash infoHash{};
    auto *port = m_port.data();

    QMetaObject::invokeMethod(port, [&infoHash, &port] {
        infoHash = port->info();
    }, Qt::BlockingQueuedConnection);
    return uni_cast<sol::object>(ts, infoHash);
}

void Port::open() const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    bool status = false;
    auto *port = m_port.data();

    QMetaObject::invokeMethod(port, [&status, &port] {
        status = port->open();
    }, Qt::BlockingQueuedConnection);
    if (!status) throw sol::error("failed to open port: " + m_portName.toStdString());
}

void Port::close() const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    auto *port = m_port.data();

    QMetaObject::invokeMethod(port, [&port] {
        port->close();
    }, Qt::BlockingQueuedConnection);
}

void Port::clear() const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    auto *port = m_port.data();

    QMetaObject::invokeMethod(port, [&port] {
        port->clear();
    }, Qt::BlockingQueuedConnection);
}

void Port::write(const std::string &data, const int messageType, const std::string &peerIp) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    if (m_port->type() == PortType::BluetoothLe)
        throw sol::error(m_portName.toStdString() + ": Bluetooth LE writes require mode, service UUID and characteristic UUID");
    bool status = false;
    auto *port = m_port.data();
    const QByteArray txData(data.data(), static_cast<qsizetype>(data.size()));
    const auto peer = QString::fromStdString(peerIp);

    QMetaObject::invokeMethod(port, [&status, port, &txData, &peer, messageType] {
        switch (port->type()) {
            case PortType::TcpServer:
                status = static_cast<TcpServer *>(port)->write(txData, peer, "", "");
                break;
            case PortType::SslServer:
                status = static_cast<SslServer *>(port)->write(txData, peer, "", "");
                break;
            case PortType::WebSocketClient:
                status = static_cast<WebSocketClient *>(port)->write(txData, messageType, "", "");
                break;
            case PortType::WebSocketServer:
                status = static_cast<WebSocketServer *>(port)->write(txData, messageType, peer, "", "");
                break;
            default:
                status = port->write(txData, "", "");
                break;
        }
    }, Qt::BlockingQueuedConnection);
    if (!status) throw sol::error("failed to write port: " + m_portName.toStdString());
}

void Port::write(const std::string &data, const int mode, const std::string &serviceUuid, const std::string &characteristicUuid) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    if (mode != BluetoothLe::Capability::WriteRequest && mode != BluetoothLe::Capability::WriteCommand)
        throw sol::error(m_portName.toStdString() + ": invalid BLE write mode (expected 1 or 2)");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    const QByteArray txData(data.data(), static_cast<qsizetype>(data.size()));
    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &exception, &txData, &service, &characteristic, mode] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "UUID writes require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic, mode);
        if (!exception.isEmpty()) return;
        try {
            ble->write(txData, mode, service, characteristic);
        } catch (const std::exception &error) {
            exception = QString::fromUtf8(error.what());
        } catch (...) {
            exception = "failed to write characteristic";
        }
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
}

sol::object Port::read(const sol::this_state ts, const int length, const int timeout, const std::string &peerIp, const bool peek) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    if (m_port->type() == PortType::BluetoothLe)
        throw sol::error(m_portName.toStdString() + ": Bluetooth LE buffer reads require service UUID and characteristic UUID");
    sol::state_view lua(ts);
    QByteArray rxData{};
    auto *port = m_port.data();

    if (port->type() == PortType::Vision) {
        if (peek) throw sol::error(m_portName.toStdString() + ": vision ports do not support peek");
        QVariantList results{};
        auto *vision = static_cast<Vision *>(port);
        QMetaObject::invokeMethod(vision, [&results, vision, timeout] {
            results = vision->read(timeout);
        }, Qt::BlockingQueuedConnection);
        return uni_cast<sol::object>(ts, results);
    }
    const auto peer = QString::fromStdString(peerIp);
    QMetaObject::invokeMethod(port, [&rxData, port, length, timeout, &peer, peek] {
        if (!peer.isEmpty()) {
            switch (port->type()) {
                case PortType::TcpServer:
                    rxData = static_cast<TcpServer *>(port)->read(length, timeout, peer, "", peek);
                    return;
                case PortType::SslServer:
                    rxData = static_cast<SslServer *>(port)->read(length, timeout, peer, "", peek);
                    return;
                case PortType::WebSocketServer:
                    rxData = static_cast<WebSocketServer *>(port)->read(length, timeout, peer, "", peek);
                    return;
                default:
                    break;
            }
        }
        rxData = port->read(length, timeout, "", peek);
    }, Qt::BlockingQueuedConnection);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

sol::object Port::read(const sol::this_state ts, const std::string &serviceUuid, const std::string &characteristicUuid) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    QByteArray rxData{};
    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &rxData, &exception, &service, &characteristic] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "UUID reads require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic, BluetoothLe::Capability::Read);
        if (!exception.isEmpty()) return;
        try {
            rxData = ble->read(service, characteristic);
        } catch (const std::exception &error) {
            exception = QString::fromUtf8(error.what());
        } catch (...) {
            exception = "failed to read characteristic";
        }
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
    sol::state_view lua(ts);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

sol::object Port::read(const sol::this_state ts, const int length, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid, const bool peek) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    QByteArray rxData{};
    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &rxData, &exception, &service, &characteristic, length, timeout, peek] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "UUID buffer reads require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic);
        if (!exception.isEmpty()) return;
        rxData = ble->read(length, timeout, service, characteristic, peek);
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
    sol::state_view lua(ts);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

sol::object Port::readUntil(const sol::this_state ts, const std::string &text, const int timeout, const std::string &peerIp, const bool peek) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    if (m_port->type() == PortType::BluetoothLe)
        throw sol::error(m_portName.toStdString() + ": Bluetooth LE buffer reads require service UUID and characteristic UUID");
    sol::state_view lua(ts);
    QByteArray rxData{};
    auto *port = m_port.data();
    if (peek && port->type() == PortType::Vision) throw sol::error(m_portName.toStdString() + ": vision ports do not support peekUntil");
    const QByteArray textData(text.data(), static_cast<qsizetype>(text.size()));
    const auto peer = QString::fromStdString(peerIp);

    QMetaObject::invokeMethod(port, [&rxData, port, &textData, timeout, &peer, peek] {
        if (!peer.isEmpty()) {
            switch (port->type()) {
                case PortType::TcpServer:
                    rxData = static_cast<TcpServer *>(port)->readUntil(textData, timeout, peer, "", peek);
                    return;
                case PortType::SslServer:
                    rxData = static_cast<SslServer *>(port)->readUntil(textData, timeout, peer, "", peek);
                    return;
                case PortType::WebSocketServer:
                    rxData = static_cast<WebSocketServer *>(port)->readUntil(textData, timeout, peer, "", peek);
                    return;
                default:
                    break;
            }
        }
        rxData = port->readUntil(textData, timeout, "", peek);
    }, Qt::BlockingQueuedConnection);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

sol::object Port::readUntil(const sol::this_state ts, const std::string &text, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid, const bool peek) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    const QByteArray textData(text.data(), static_cast<qsizetype>(text.size()));
    QByteArray rxData{};
    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &rxData, &exception, &textData, &service, &characteristic, timeout, peek] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "UUID buffer reads require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic);
        if (!exception.isEmpty()) return;
        rxData = ble->readUntil(textData, timeout, service, characteristic, peek);
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
    sol::state_view lua(ts);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

void Port::subscribe(const int mode, const std::string &serviceUuid, const std::string &characteristicUuid) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    if (mode != BluetoothLe::Capability::Notify && mode != BluetoothLe::Capability::Indicate)
        throw sol::error(m_portName.toStdString() + ": invalid BLE subscription mode (expected 3 or 4)");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &exception, &service, &characteristic, mode] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "subscriptions require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic, mode);
        if (!exception.isEmpty()) return;
        try {
            ble->subscribe(mode, service, characteristic);
        } catch (const std::exception &error) {
            exception = QString::fromUtf8(error.what());
        } catch (...) {
            exception = "failed to subscribe characteristic";
        }
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
}

void Port::unsubscribe(const std::string &serviceUuid, const std::string &characteristicUuid) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    const auto service = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(serviceUuid)).value;
    const auto characteristic = uni_cast<QBluetoothUuidNormalized>(QString::fromStdString(characteristicUuid)).value;
    if (service.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid service UUID");
    if (characteristic.isEmpty())
        throw sol::error(m_portName.toStdString() + ": invalid characteristic UUID");

    QString exception{};
    auto *port = m_port.data();
    QMetaObject::invokeMethod(port, [port, &exception, &service, &characteristic] {
        if (port->type() != PortType::BluetoothLe) {
            exception = "subscriptions require a Bluetooth LE port";
            return;
        }
        auto *ble = static_cast<BluetoothLe *>(port);
        exception = ble->check(service, characteristic);
        if (!exception.isEmpty()) return;
        try {
            ble->unsubscribe(service, characteristic);
        } catch (const std::exception &error) {
            exception = QString::fromUtf8(error.what());
        } catch (...) {
            exception = "failed to unsubscribe characteristic";
        }
    }, Qt::BlockingQueuedConnection);
    if (!exception.isEmpty()) throw sol::error(m_portName.toStdString() + ": " + exception.toStdString());
}

#include "api/port.h"

#include <sol/state_view.hpp>
#include <sol/table_core.hpp>

#include <utility>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "port/vision.h"
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

void Port::write(const std::string &data, const std::string &peerIp) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    bool status = false;
    auto *port = m_port.data();
    const QByteArray txData(data.data(), static_cast<qsizetype>(data.size()));

    if (port->type() == PortType::TcpServer || port->type() == PortType::SslServer || port->type() == PortType::WebSocketServer) {
        QMetaObject::invokeMethod(port, [&status, &port, &txData, &peerIp] {
            status = port->write(txData, QString::fromStdString(peerIp), "", "");
        }, Qt::BlockingQueuedConnection);
    } else {
        QMetaObject::invokeMethod(port, [&status, &port, &txData] {
            status = port->write(txData, "", "");
        }, Qt::BlockingQueuedConnection);
    }
    if (!status) throw sol::error("failed to write port: " + m_portName.toStdString());
}

sol::object Port::read(const sol::this_state ts, const int length, const int timeout, const std::string &peerIp) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    sol::state_view lua(ts);
    QByteArray rxData{};
    auto *port = m_port.data();

    if (port->type() == PortType::TcpServer || port->type() == PortType::SslServer || port->type() == PortType::WebSocketServer) {
        QMetaObject::invokeMethod(port, [&rxData, &port, &length, &timeout, &peerIp] {
            if (peerIp.empty()) rxData = port->read(length, timeout, "");
            else rxData = port->read(length, timeout, QString::fromStdString(peerIp), "");
        }, Qt::BlockingQueuedConnection);
        return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
    }
    if (port->type() == PortType::Vision) {
        QVariantList results{};
        auto *vision = static_cast<Vision *>(port);
        QMetaObject::invokeMethod(vision, [&results, &vision, &timeout] {
            results = vision->result(timeout);
        }, Qt::BlockingQueuedConnection);
        return uni_cast<sol::object>(ts, results);
    }
    QMetaObject::invokeMethod(port, [&rxData, &port, &length, &timeout] {
        rxData = port->read(length, timeout, "");
    }, Qt::BlockingQueuedConnection);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

sol::object Port::readUntil(const sol::this_state ts, const std::string &text, const int timeout, const std::string &peerIp) const {
    if (m_port.isNull()) throw sol::error(m_portName.toStdString() + ": port is no longer available");
    sol::state_view lua(ts);
    QByteArray rxData{};
    auto *port = m_port.data();
    const QByteArray textData(text.data(), static_cast<qsizetype>(text.size()));

    if (port->type() == PortType::TcpServer || port->type() == PortType::SslServer || port->type() == PortType::WebSocketServer) {
        QMetaObject::invokeMethod(port, [&rxData, &port, &textData, &timeout, &peerIp] {
            if (peerIp.empty()) rxData = port->readUntil(textData, timeout, "");
            else rxData = port->readUntil(textData, timeout, QString::fromStdString(peerIp), "");
        }, Qt::BlockingQueuedConnection);
        return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
    }
    QMetaObject::invokeMethod(port, [&rxData, &port, &textData, &timeout] {
        rxData = port->readUntil(textData, timeout, "");
    }, Qt::BlockingQueuedConnection);
    return sol::make_object(lua, std::string(rxData.constData(), static_cast<std::string::size_type>(rxData.size())));
}

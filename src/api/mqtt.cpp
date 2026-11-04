#include "api/mqtt.h"

#include <QDeadlineTimer>
#include <sol/error.hpp>
#include <sol/state_view.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"

// public
Mqtt::Mqtt(QObject *parent)
    : QObject(parent) {
}

void Mqtt::init(const std::string &portName, const int timeout) {
    const auto port = g_port->m_portHash.constFind(QString::fromStdString(portName));
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");
    m_portName = portName;
    m_timeout = timeout;
    m_port = port.value();

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->open()) return "open failed";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Mqtt::connect(const std::string &clientId, const sol::optional<std::string> &username, const sol::optional<std::string> &password, const sol::optional<int> keepAlive,
                   const sol::optional<bool> cleanSession) {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    const auto _keepAlive = keepAlive.value_or(60);
    const auto _cleanSession = cleanSession.value_or(true);
    if (_keepAlive < 0 || _keepAlive > 65535) throw sol::error(m_portName + ": keepAlive must be between 0 and 65535");
    if (clientId.empty() && !_cleanSession) throw sol::error(m_portName + ": clientId must not be empty when cleanSession is false");
    if (password.has_value() && !username.has_value()) throw sol::error(m_portName + ": password requires a username");

    quint8 connectFlags = _cleanSession ? 0x02 : 0x00;
    if (username.has_value()) connectFlags |= 0x80;
    if (password.has_value()) connectFlags |= 0x40;

    QByteArray data("\x00\x04MQTT\x04", 7);
    data.append(static_cast<char>(connectFlags));
    data.append(static_cast<char>(_keepAlive >> 8 & 0xFF));
    data.append(static_cast<char>(_keepAlive & 0xFF));

    const auto _clientId = QByteArray::fromStdString(clientId);
    if (_clientId.size() > 65535) throw sol::error(m_portName + ": clientId exceeds 65535 bytes");
    if (_clientId.contains('\0') || !_clientId.isValidUtf8()) throw sol::error(m_portName + ": clientId must be valid UTF-8 without null bytes");
    data.append(static_cast<char>(_clientId.size() >> 8 & 0xFF));
    data.append(static_cast<char>(_clientId.size() & 0xFF));
    data += _clientId;

    if (username.has_value()) {
        const auto _username = QByteArray::fromStdString(username.value());
        if (_username.size() > 65535) throw sol::error(m_portName + ": username exceeds 65535 bytes");
        if (_username.contains('\0') || !_username.isValidUtf8()) throw sol::error(m_portName + ": username must be valid UTF-8 without null bytes");
        data.append(static_cast<char>(_username.size() >> 8 & 0xFF));
        data.append(static_cast<char>(_username.size() & 0xFF));
        data += _username;
    }

    if (password.has_value()) {
        const auto _password = QByteArray::fromStdString(password.value());
        if (_password.size() > 65535) throw sol::error(m_portName + ": password exceeds 65535 bytes");
        data.append(static_cast<char>(_password.size() >> 8 & 0xFF));
        data.append(static_cast<char>(_password.size() & 0xFF));
        data += _password;
    }

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this, &data, _cleanSession]() -> QString {
        m_messages.clear();
        auto exception = request(ControlPacketType::Connect, 0, data);
        if (!exception.isEmpty()) return exception;

        const auto result = response(ControlPacketType::Connect);
        if (!result.exception.isEmpty()) return result.exception;
        if (result.type != ControlPacketType::Connack) return "unexpected mqtt response(" + QString::number(result.type) + ")";
        if (result.data.size() != 2) return "invalid mqtt connack length";

        const auto connackFlags = static_cast<quint8>(result.data.at(0));
        const auto code = static_cast<quint8>(result.data.at(1));
        if (connackFlags > 1) return "invalid mqtt connack flags";
        if (connackFlags != 0 && (_cleanSession || code != ConnectReturnCode::ConnectionAccepted)) return "invalid mqtt session present flag";

        switch (code) {
            case ConnectReturnCode::ConnectionAccepted: return {};
            case ConnectReturnCode::UnacceptableProtocolVersion: return "connect rejected(1): unacceptable protocol version";
            case ConnectReturnCode::IdentifierRejected: return "connect rejected(2): identifier rejected";
            case ConnectReturnCode::ServerUnavailable: return "connect rejected(3): server unavailable";
            case ConnectReturnCode::BadUsernameOrPassword: return "connect rejected(4): bad user name or password";
            case ConnectReturnCode::NotAuthorized: return "connect rejected(5): not authorized";
            default: return "invalid mqtt connect return code(" + QString::number(code) + ")";
        }
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

int Mqtt::subscribe(const std::string &topicFilter, const sol::optional<int> qos) {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    const auto _qos = qos.value_or(QoS::AtMostOnce);
    if (_qos < QoS::AtMostOnce || _qos > QoS::ExactlyOnce) throw sol::error(m_portName + ": qos must be between 0 and 2");

    const auto _topicFilter = QByteArray::fromStdString(topicFilter);
    if (_topicFilter.isEmpty()) throw sol::error(m_portName + ": topicFilter must not be empty");
    if (_topicFilter.size() > 65535) throw sol::error(m_portName + ": topicFilter exceeds 65535 bytes");
    if (_topicFilter.contains('\0') || !_topicFilter.isValidUtf8()) throw sol::error(m_portName + ": topicFilter must be valid UTF-8 without null bytes");

    QString exception{};
    int grantedQos{-1};

    QMetaObject::invokeMethod(m_port, [&grantedQos, this, &_topicFilter, _qos]() -> QString {
        if (++m_packetId == 0) ++m_packetId;
        const auto packetId = m_packetId;

        QByteArray data{};
        data.append(static_cast<char>(packetId >> 8 & 0xFF));
        data.append(static_cast<char>(packetId & 0xFF));
        data.append(static_cast<char>(_topicFilter.size() >> 8 & 0xFF));
        data.append(static_cast<char>(_topicFilter.size() & 0xFF));
        data += _topicFilter;
        data.append(static_cast<char>(_qos));

        auto exception = request(ControlPacketType::Subscribe, 0x02, data);
        if (!exception.isEmpty()) return exception;

        const auto result = response(ControlPacketType::Subscribe);
        if (!result.exception.isEmpty()) return result.exception;
        if (result.type != ControlPacketType::Suback) return "unexpected mqtt response(" + QString::number(result.type) + ")";
        if (result.data.size() != 3) return "invalid mqtt suback length";

        const auto responsePacketId = static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1));
        if (responsePacketId != packetId) return "packet identifier mismatch";

        const auto code = static_cast<quint8>(result.data.at(2));
        if (code == SubscribeReturnCode::Failure) return "subscribe rejected(128)";
        if (code > QoS::ExactlyOnce) return "invalid mqtt suback return code(" + QString::number(code) + ")";
        if (code > _qos) return "granted qos exceeds requested qos";

        grantedQos = code;
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());

    return grantedQos;
}

void Mqtt::publish(const std::string &topic, const std::string &payload, const sol::optional<int> qos, const sol::optional<bool> retain) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    const auto _qos = qos.value_or(QoS::AtMostOnce);
    const auto _retain = retain.value_or(false);
    if (_qos != QoS::AtMostOnce) throw sol::error(m_portName + ": publish only supports qos 0");

    const auto _topic = QByteArray::fromStdString(topic);
    if (_topic.isEmpty()) throw sol::error(m_portName + ": topic must not be empty");
    if (_topic.size() > 65535) throw sol::error(m_portName + ": topic exceeds 65535 bytes");
    if (_topic.contains('\0') || !_topic.isValidUtf8()) throw sol::error(m_portName + ": topic must be valid UTF-8 without null bytes");

    QByteArray data{};
    data.append(static_cast<char>(_topic.size() >> 8 & 0xFF));
    data.append(static_cast<char>(_topic.size() & 0xFF));
    data += _topic;
    data += QByteArray::fromStdString(payload);

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this, &data, _retain]() -> QString {
        return request(ControlPacketType::Publish, _retain ? 0x01 : 0, data);
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

sol::object Mqtt::consume(const sol::this_state ts) {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");

    Result result{};
    QMetaObject::invokeMethod(m_port, [&result, this] {
        if (!m_messages.isEmpty()) result = m_messages.dequeue();
        else result = response();
    }, Qt::BlockingQueuedConnection);

    sol::state_view lua(ts);
    if (result.exception == "read timeout") return sol::make_object(lua, sol::nil);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());
    if (result.type != ControlPacketType::Publish) throw sol::error(m_portName + ": unexpected mqtt message type(" + std::to_string(result.type) + ")");
    if ((result.flags >> 1 & 0x03) != QoS::AtMostOnce) throw sol::error(m_portName + ": consume only supports qos 0");
    if (result.data.size() < 2) throw sol::error(m_portName + ": invalid mqtt publish topic length");

    const int topicLength = static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1));
    if (topicLength == 0 || topicLength > result.data.size() - 2) throw sol::error(m_portName + ": invalid mqtt publish topic length");
    const auto topic = result.data.sliced(2, topicLength);
    if (topic.contains('\0') || !topic.isValidUtf8()) throw sol::error(m_portName + ": topic must be valid UTF-8 without null bytes");
    if (topic.contains('+') || topic.contains('#')) throw sol::error(m_portName + ": mqtt publish topic must not contain wildcards");

    const auto payloadOffset = topicLength + 2;
    auto message = lua.create_table();
    message["topic"] = topic.toStdString();
    message["payload"] = std::string(result.data.constData() + payloadOffset, static_cast<std::string::size_type>(result.data.size() - payloadOffset));
    return sol::make_object(lua, message);
}

// private
QString Mqtt::request(const int type, const quint8 flags, const QByteArray &data) const {
    if (type < ControlPacketType::Connect || type > ControlPacketType::Disconnect) return "invalid mqtt control packet type";
    if (flags > 0x0F) return "invalid mqtt fixed header flags";
    if (data.size() > 268435455) return "mqtt packet too large";

    QByteArray txData(1, static_cast<char>(type << 4 | flags));
    auto size = data.size();
    do {
        auto byte = static_cast<quint8>(size & 0x7F);
        size >>= 7;
        if (size > 0) byte |= 0x80;
        txData.append(static_cast<char>(byte));
    } while (size > 0);
    txData += data;

    if (!m_port->write(txData, "hex", "null")) return "write failed";

    return {};
}

Mqtt::Result Mqtt::response(const std::optional<int> type) {
    if (m_port.isNull()) return {.exception = "port is no longer available"};
    const QDeadlineTimer deadline(m_timeout);

    while (true) {
        int size{};
        int headerSize{};
        for (int index = 1; index <= 4; ++index) {
            const auto header = m_port->read(index + 1, static_cast<int>(deadline.remainingTime()), "hex", true);
            if (header.isEmpty()) return {.exception = "read timeout"};

            const auto byte = static_cast<quint8>(header.at(index));
            size |= (byte & 0x7F) << ((index - 1) * 7);
            if (!(byte & 0x80)) {
                headerSize = index + 1;
                break;
            }
            if (index == 4) return {.exception = "invalid mqtt remaining length; reconnect required"};
        }

        const auto rxData = m_port->read(headerSize + size, static_cast<int>(deadline.remainingTime()), "hex");
        if (rxData.isEmpty()) return {.exception = "read timeout"};
        auto result = parser(rxData);
        if (!result.exception.isEmpty()) return result;
        if (type.has_value() && type.value() != ControlPacketType::Connect && result.type == ControlPacketType::Publish) {
            m_messages.enqueue(result);
            if (deadline.hasExpired()) return {.exception = "read timeout"};
            continue;
        }
        return result;
    }
}

Mqtt::Result Mqtt::parser(const QByteArray &rxData) {
    if (rxData.size() < 2) return {.exception = "invalid mqtt packet length"};

    const auto header = static_cast<quint8>(rxData.at(0));
    const int type = header >> 4;
    const quint8 flags = header & 0x0F;
    if (type < ControlPacketType::Connect || type > ControlPacketType::Disconnect) return {.exception = "invalid mqtt control packet type"};

    if (type == ControlPacketType::Publish) {
        const auto qos = flags >> 1 & 0x03;
        if (qos > QoS::ExactlyOnce || (qos == QoS::AtMostOnce && (flags & 0x08))) return {.exception = "invalid mqtt fixed header flags"};
    } else if (type == ControlPacketType::Pubrel || type == ControlPacketType::Subscribe || type == ControlPacketType::Unsubscribe) {
        if (flags != 0x02) return {.exception = "invalid mqtt fixed header flags"};
    } else if (flags != 0) return {.exception = "invalid mqtt fixed header flags"};

    int size{};
    for (int index = 1; index <= 4; ++index) {
        if (rxData.size() <= index) return {.exception = "invalid mqtt remaining length"};
        const auto byte = static_cast<quint8>(rxData.at(index));
        size |= (byte & 0x7F) << ((index - 1) * 7);
        if (byte & 0x80) continue;

        const auto headerSize = index + 1;
        if (rxData.size() - headerSize != size) return {.exception = "invalid mqtt packet length"};

        return {.type = type, .flags = flags, .data = rxData.sliced(headerSize, size), .exception = {}};
    }
    return {.exception = "invalid mqtt remaining length"};
}

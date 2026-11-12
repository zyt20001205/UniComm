#include "port/webSocketServer.h"

#include <QScopedValueRollback>
#include <QWebSocketServer>

#include "globals.h"
#include "port/module/ringBuffer.h"
#include "port/webSocketClient.h"
#include "util/uniCast.h"

// public
WebSocketServer::WebSocketServer(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig) {
}

WebSocketServer::~WebSocketServer() {
    close();
}

int WebSocketServer::type() {
    return PortType::WebSocketServer;
}

QJsonObject WebSocketServer::config() {
    return m_portConfig;
}

QVariantHash WebSocketServer::info() {
    QVariantHash infoHash = {
        {"status", m_webSocketServer && m_webSocketServer->isListening() ? "opened" : "closed"},
        {"localHost", m_portConfig["localHost"].toString()},
        {"localPort", QString::number(m_portConfig["localPort"].toInt())}
    };
    int index = 1;
    for (const auto &peerIp: m_peerHash.keys()) {
        const auto *peer = m_peerHash.value(peerIp);
        auto *buffer = m_bufferHash.value(peerIp);
        infoHash.insert("peer" + QString::number(index++), QVariantHash{
            {"peerIp", peerIp},
            {"peerAddress", peer->peerAddress().toString()},
            {"peerPort", peer->peerPort()},
            {"used", QString::number(buffer->used())},
            {"capacity", QString::number(m_portConfig["bufferSize"].toInt())}
        });
    }
    return infoHash;
}

bool WebSocketServer::open() {
    if (m_webSocketServer == nullptr) {
        // TODO: Restore WSS when TLS settings are unified across ports.
        m_webSocketServer = new QWebSocketServer(m_portConfig["portName"].toString(), QWebSocketServer::NonSecureMode, this);
        connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::handleNewConnection);
        connect(m_webSocketServer, &QWebSocketServer::acceptError, this, &WebSocketServer::handleServerError);
        connect(m_webSocketServer, &QWebSocketServer::serverError, this, &WebSocketServer::handleServerError);
    }
    if (m_webSocketServer->isListening()) return true;

    if (m_webSocketServer->listen(QHostAddress(m_portConfig["localHost"].toString()), m_portConfig["localPort"].toInt())) {
        emit refreshPort(m_portConfig["portName"].toString(), true);
        emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("started on %1:%2").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt())));
        return true;
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("open failed: %1").arg(m_webSocketServer->errorString()));
    return false;
}

void WebSocketServer::close() {
    if (m_webSocketServer == nullptr) return;

    m_webSocketServer->close();
    const auto peers = m_peerHash.values();
    m_peerHash.clear();
    qDeleteAll(m_bufferHash);
    m_bufferHash.clear();
    for (auto *peer: peers) {
        if (peer == nullptr) continue;
        disconnect(peer, nullptr, this, nullptr);
        peer->close();
        peer->deleteLater();
    }
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void WebSocketServer::clear() {
    for (auto *buffer: m_bufferHash) buffer->clear();
}

bool WebSocketServer::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    return write(txData, WebSocketMessageType::Binary, "", logFormat, txSuffix);
}

bool WebSocketServer::write(const QByteArray &txData, const int messageType, const QString &peerIp, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData, messageType, peerIp);
}

QByteArray WebSocketServer::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleRead(length, timeout, m_peerHash.keys().first(), peek);
}

QByteArray WebSocketServer::read(const int length, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peerIp, peek);
}

QByteArray WebSocketServer::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleReadUntil(text, timeout, m_peerHash.keys().first(), peek);
}

QByteArray WebSocketServer::readUntil(const QByteArray &text, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peerIp, peek);
}

// private
void WebSocketServer::handleNewConnection() {
    while (m_webSocketServer->hasPendingConnections()) {
        auto *peer = m_webSocketServer->nextPendingConnection();
        if (peer == nullptr) continue;
        peer->setParent(this);
        const QString peerIp = peer->peerAddress().toString() + ":" + QString::number(peer->peerPort());
        peer->setProperty("peerIp", peerIp);
        connect(peer, &QWebSocket::binaryMessageReceived, this, [this, peer](const QByteArray &message) { handleMessage(peer, message); });
        connect(peer, &QWebSocket::textMessageReceived, this, [this, peer](const QString &message) { handleMessage(peer, message.toUtf8()); });
        connect(peer, &QWebSocket::stateChanged, this, [this, peer](QAbstractSocket::SocketState state) { handleStateChanged(peer, state); });
        connect(peer, &QWebSocket::errorOccurred, this, [this, peer] { handleError(peer); });
        handleStateChanged(peer, peer->state());
    }
}

void WebSocketServer::handleServerError() {
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), m_webSocketServer->errorString());
}

void WebSocketServer::handleStateChanged(QWebSocket *webSocketServerPeer, const QAbstractSocket::SocketState state) {
    const QString peerIp = webSocketServerPeer->property("peerIp").toString();
    QString message{};
    switch (state) {
        case QAbstractSocket::UnconnectedState: {
            const auto peer = m_peerHash.constFind(peerIp);
            if (peer == m_peerHash.constEnd() || peer.value() != webSocketServerPeer) {
                webSocketServerPeer->deleteLater();
                return;
            }
            message = QString("lost connection from %1 (close code %2)").arg(peerIp).arg(static_cast<int>(webSocketServerPeer->closeCode()));
            const auto reason = webSocketServerPeer->closeReason();
            if (!reason.isEmpty()) message += ": " + reason;
            m_peerHash.remove(peerIp);
            delete m_bufferHash.take(peerIp);
            webSocketServerPeer->deleteLater();
        }
        break;
        case QAbstractSocket::ConnectedState:
            m_peerHash.insert(peerIp, webSocketServerPeer);
            m_bufferHash.insert(peerIp, new RingBuffer(m_portConfig["bufferSize"].toInt()));
            message = QString("accepts connection from %1").arg(peerIp);
            break;
        case QAbstractSocket::ClosingState:
            message = QString("disconnecting from %1").arg(peerIp);
            break;
        default: return;
    }
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void WebSocketServer::handleMessage(QWebSocket *webSocketServerPeer, const QByteArray &message) {
    const QString peerIp = webSocketServerPeer->property("peerIp").toString();
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    if (buffer == nullptr) return;
    if (buffer->write(message) != message.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), "buffer overflow");
        webSocketServerPeer->close();
        return;
    }
    handleLog(LogLevel::Receive, message, webSocketServerPeer);
}

void WebSocketServer::handleError(const QWebSocket *webSocketServerPeer) {
    if (webSocketServerPeer == nullptr || webSocketServerPeer->error() == QAbstractSocket::SocketTimeoutError) return;
    emit appendLog(LogLevel::Error, QString("[%1]").arg(webSocketServerPeer->property("peerIp").toString()), QString("error: %1").arg(webSocketServerPeer->errorString()));
}

bool WebSocketServer::handleWrite(const QByteArray &f_txData, const int messageType, const QString &peerIp) {
    if (messageType != WebSocketMessageType::Text && messageType != WebSocketMessageType::Binary) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "invalid message type");
        return false;
    }
    if (messageType == WebSocketMessageType::Text && !f_txData.isValidUtf8()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "invalid UTF-8 text");
        return false;
    }
    if (m_webSocketServer == nullptr || !m_webSocketServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    const auto send = [this, &f_txData, messageType](QWebSocket *peer) {
        if (peer->state() != QAbstractSocket::ConnectedState) return false;
        const auto written = messageType == WebSocketMessageType::Text
                                 ? peer->sendTextMessage(QString::fromUtf8(f_txData))
                                 : peer->sendBinaryMessage(f_txData);
        if (written != f_txData.size()) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(peer->property("peerIp").toString()), "write failed");
            return false;
        }
        handleLog(LogLevel::Transmit, f_txData, peer);
        return true;
    };
    if (peerIp.isEmpty()) {
        bool status = true;
        for (auto *peer: m_peerHash) if (!send(peer)) status = false;
        return status;
    }
    const auto peer = m_peerHash.constFind(peerIp);
    if (peer == m_peerHash.constEnd()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
        return false;
    }
    return send(peer.value());
}

QByteArray WebSocketServer::handleRead(const int length, const int timeout, const QString &peerIp, const bool peek) {
    if (m_webSocketServer == nullptr || !m_webSocketServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    auto *peer = m_peerHash.value(peerIp, nullptr);
    if (buffer == nullptr || peer == nullptr) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
        return {};
    }
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    while (buffer->used() < minimum) {
        if (peer->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        UWebSocket::waitForReadyRead(peer, 10);
        buffer = m_bufferHash.value(peerIp, nullptr);
        peer = m_peerHash.value(peerIp, nullptr);
        if (buffer == nullptr || peer == nullptr) return {};
    }
    return buffer->read(length, peek);
}

QByteArray WebSocketServer::handleReadUntil(const QByteArray &text, const int timeout, const QString &peerIp, const bool peek) {
    if (m_webSocketServer == nullptr || !m_webSocketServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    if (text.isEmpty()) return {};
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    auto *peer = m_peerHash.value(peerIp, nullptr);
    if (buffer == nullptr || peer == nullptr) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
        return {};
    }
    const QDeadlineTimer deadline(timeout);
    QByteArray data = buffer->readUntil(text, peek);
    while (data.isEmpty()) {
        if (peer->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        UWebSocket::waitForReadyRead(peer, 10);
        buffer = m_bufferHash.value(peerIp, nullptr);
        peer = m_peerHash.value(peerIp, nullptr);
        if (buffer == nullptr || peer == nullptr) return {};
        data = buffer->readUntil(text, peek);
    }
    return data;
}

void WebSocketServer::handleLog(const int type, const QByteArray &data, const QWebSocket *webSocketServerPeer) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    const QString peerIp = webSocketServerPeer->property("peerIp").toString();
    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    }
}

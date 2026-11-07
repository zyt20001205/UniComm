#include "port/webSocketServer.h"

#include <QEventLoop>
#include <QFile>
#include <QScopedValueRollback>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>

#include "globals.h"
#include "port/module/ringBuffer.h"
#include "util/uniCast.h"

namespace {
void waitForMessage(QWebSocket *socket) {
    QEventLoop eventLoop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(socket, &QWebSocket::binaryMessageReceived, &eventLoop, &QEventLoop::quit);
    QObject::connect(socket, &QWebSocket::textMessageReceived, &eventLoop, &QEventLoop::quit);
    QObject::connect(socket, &QWebSocket::disconnected, &eventLoop, &QEventLoop::quit);
    timer.start(10);
    eventLoop.exec();
}
}

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
    const QString status = m_webSocketServer && m_webSocketServer->isListening() ? "opened" : "closed";
    QVariantHash infoHash = {
        {"status", status},
        {"localHost", m_portConfig["localHost"].toString()},
        {"localPort", QString::number(m_portConfig["localPort"].toInt())},
        {"secure", m_portConfig["secure"].toBool()}
    };
    const int capacity = m_portConfig["bufferSize"].toInt();
    int index = 1;
    for (const auto &peerIp: m_peerHash.keys()) {
        const auto *peer = m_peerHash.value(peerIp);
        auto *buffer = m_bufferHash.value(peerIp);
        const QVariantHash peerInfoHash = {
            {"peerAddress", peer->peerAddress().toString()},
            {"peerPort", peer->peerPort()},
            {"used", QString::number(buffer->used())},
            {"capacity", QString::number(capacity)}
        };
        infoHash.insert("peer" + QString::number(index++), peerInfoHash);
    }
    return infoHash;
}

bool WebSocketServer::open() {
    if (m_webSocketServer == nullptr) {
        const auto mode = m_portConfig["secure"].toBool()
                              ? QWebSocketServer::SecureMode
                              : QWebSocketServer::NonSecureMode;
        m_webSocketServer = new QWebSocketServer(m_portConfig["portName"].toString(), mode, this);
        connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::handleNewConnection);
        connect(m_webSocketServer, &QWebSocketServer::acceptError, this, &WebSocketServer::handleServerError);
        connect(m_webSocketServer, &QWebSocketServer::serverError, this, &WebSocketServer::handleServerError);
    }
    if (m_webSocketServer->isListening()) return true;
    if (m_portConfig["secure"].toBool() && !configureSsl()) return false;

    if (m_webSocketServer->listen(QHostAddress(m_portConfig["localHost"].toString()), m_portConfig["localPort"].toInt())) {
        const QVariantHash session{{"active", true}};
        emit refreshPort(m_portConfig["portName"].toString(), session);
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
    const QVariantHash session{{"active", false}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void WebSocketServer::disconnectPeer(const QString &peerIp) {
    const auto peer = m_peerHash.constFind(peerIp);
    if (peer == m_peerHash.constEnd()) return;
    peer.value()->close();
}

void WebSocketServer::clear() {
    for (auto *buffer: m_bufferHash) buffer->clear();
}

void WebSocketServer::monitor(const bool enabled) {
    Q_UNUSED(enabled);
    // TODO: Aggregate statistics across active and disconnected peer buffers.
}

bool WebSocketServer::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData);
}

bool WebSocketServer::write(const QByteArray &txData, const QString &peerIp, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData, peerIp);
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
bool WebSocketServer::configureSsl() {
    QFile certificateFile(m_portConfig["certificate"].toString());
    if (!certificateFile.open(QIODevice::ReadOnly)) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "certificate invalid");
        return false;
    }
    const auto certificates = QSslCertificate::fromDevice(&certificateFile, QSsl::Pem);
    if (certificates.isEmpty()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "certificate invalid");
        return false;
    }

    QFile privateKeyFile(m_portConfig["privateKey"].toString());
    if (!privateKeyFile.open(QIODevice::ReadOnly)) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "private key invalid");
        return false;
    }
    const QSslKey privateKey(&privateKeyFile,
                             certificates.first().publicKey().algorithm(),
                             QSsl::Pem,
                             QSsl::PrivateKey);
    if (privateKey.isNull()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "private key invalid");
        return false;
    }

    auto sslConfiguration = QSslConfiguration::defaultConfiguration();
    sslConfiguration.setLocalCertificateChain(certificates);
    sslConfiguration.setPrivateKey(privateKey);
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    m_webSocketServer->setSslConfiguration(sslConfiguration);
    return true;
}

void WebSocketServer::handleNewConnection() {
    while (m_webSocketServer->hasPendingConnections()) {
        auto *peer = m_webSocketServer->nextPendingConnection();
        if (peer == nullptr) continue;
        handleConnected(peer);
        connect(peer, &QWebSocket::binaryMessageReceived, this, [this, peer](const QByteArray &message) { handleMessage(peer, message); });
        connect(peer, &QWebSocket::textMessageReceived, this, [this, peer](const QString &message) { handleMessage(peer, message.toUtf8()); });
        connect(peer, &QWebSocket::disconnected, this, [this, peer] { handleDisconnected(peer); });
        connect(peer, &QWebSocket::errorOccurred, this, [this, peer] { handleError(peer); });
    }
}

void WebSocketServer::handleServerError() {
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), m_webSocketServer->errorString());
}

void WebSocketServer::handleConnected(QWebSocket *webSocketServerPeer) {
    const QString peerIp = webSocketServerPeer->peerAddress().toString() + ":" + QString::number(webSocketServerPeer->peerPort());
    webSocketServerPeer->setProperty("peerIp", peerIp);
    m_peerHash.insert(peerIp, webSocketServerPeer);
    m_bufferHash.insert(peerIp, new RingBuffer(m_portConfig["bufferSize"].toInt()));
    emit connected(peerIp);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("accepts connection from %1").arg(peerIp));
}

void WebSocketServer::handleDisconnected(QWebSocket *webSocketServerPeer) {
    const QString peerIp = webSocketServerPeer->property("peerIp").toString();
    m_peerHash.remove(peerIp);
    webSocketServerPeer->deleteLater();
    delete m_bufferHash.take(peerIp);
    emit disconnected(peerIp);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("lost connection from %1").arg(peerIp));
}

void WebSocketServer::handleMessage(QWebSocket *webSocketServerPeer, const QByteArray &message) {
    const QString peerIp = webSocketServerPeer->property("peerIp").toString();
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    if (buffer == nullptr) return;
    if (buffer->write(message) != message.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), "buffer overflow");
        close();
        return;
    }
    emit readyRead(peerIp);
    handleLog(LogLevel::Receive, message, webSocketServerPeer);
}

void WebSocketServer::handleError(const QWebSocket *webSocketServerPeer) {
    if (webSocketServerPeer == nullptr || webSocketServerPeer->error() == QAbstractSocket::SocketTimeoutError) return;
    emit appendLog(LogLevel::Error, QString("[%1]").arg(webSocketServerPeer->property("peerIp").toString()), QString("error: %1").arg(webSocketServerPeer->errorString()));
}

bool WebSocketServer::handleWrite(const QByteArray &f_txData, const QString &peerIp) {
    if (m_webSocketServer == nullptr || !m_webSocketServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    const auto send = [this, &f_txData](QWebSocket *peer) {
        if (m_portConfig["messageType"].toString() == "text")
            peer->sendTextMessage(QString::fromUtf8(f_txData));
        else
            peer->sendBinaryMessage(f_txData);
        handleLog(LogLevel::Transmit, f_txData, peer);
    };
    if (peerIp.isEmpty()) {
        for (auto *peer: m_peerHash) send(peer);
    } else {
        const auto peer = m_peerHash.constFind(peerIp);
        if (peer == m_peerHash.constEnd()) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
            return false;
        }
        send(peer.value());
    }
    return true;
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
        if (deadline.hasExpired()) break;
        waitForMessage(peer);
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
        if (deadline.hasExpired()) break;
        waitForMessage(peer);
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

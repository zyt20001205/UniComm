#include "port/sslServer.h"

#include <QFile>
#include <QScopedValueRollback>
#include <QSslKey>
#include <QSslServer>
#include <QSslSocket>
#include <QTcpServer>

#include "globals.h"
#include "port/module/ringBuffer.h"
#include "util/uniCast.h"

// public
SslServer::SslServer(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig) {
}

SslServer::~SslServer() {
    close();
}

int SslServer::type() {
    return PortType::SslServer;
}

QJsonObject SslServer::config() {
    return m_portConfig;
}

QVariantHash SslServer::info() {
    QVariantHash infoHash = {
        {"status", m_sslServer && m_sslServer->isListening() ? "opened" : "closed"},
        {"localHost", m_portConfig["localHost"].toString()},
        {"localPort", QString::number(m_portConfig["localPort"].toInt())}
    };
    int index = 1;
    for (const auto &peerIp: m_peerHash.keys()) {
        const auto *peer = m_peerHash.value(peerIp);
        auto *buffer = m_bufferHash.value(peerIp);
        infoHash.insert("peer" + QString::number(index++), QVariantHash{
            {"peerAddress", peer->peerAddress().toString()},
            {"peerPort", peer->peerPort()},
            {"used", QString::number(buffer->used())},
            {"capacity", QString::number(m_portConfig["bufferSize"].toInt())}
        });
    }
    return infoHash;
}

bool SslServer::open() {
    if (m_sslServer == nullptr) {
        m_sslServer = new QSslServer(this);
        connect(m_sslServer, &QTcpServer::pendingConnectionAvailable, this, &SslServer::handleNewConnection);
        connect(m_sslServer, &QTcpServer::acceptError, this, &SslServer::handleServerError);
        connect(m_sslServer, &QSslServer::errorOccurred, this, [this](const QSslSocket *socket) { handleError(socket); });
    }
    if (m_sslServer->isListening()) return true;
    if (!configureSsl()) return false;

    if (m_sslServer->listen(QHostAddress(m_portConfig["localHost"].toString()), m_portConfig["localPort"].toInt())) {
        emit refreshPort(m_portConfig["portName"].toString(), true);
        emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("started on %1:%2").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt())));
        return true;
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("open failed: %1").arg(m_sslServer->errorString()));
    return false;
}

void SslServer::close() {
    if (m_sslServer == nullptr) return;

    m_sslServer->close();
    const auto peers = m_peerHash.values();
    for (auto *peer: peers) {
        if (peer == nullptr) continue;
        peer->disconnectFromHost();
        if (peer->state() != QAbstractSocket::UnconnectedState) peer->waitForDisconnected(1000);
        peer->deleteLater();
    }
    m_peerHash.clear();
    qDeleteAll(m_bufferHash);
    m_bufferHash.clear();
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void SslServer::clear() {
    for (auto *buffer: m_bufferHash) buffer->clear();
}

bool SslServer::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData);
}

bool SslServer::write(const QByteArray &txData, const QString &peerIp, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData, peerIp);
}

QByteArray SslServer::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleRead(length, timeout, m_peerHash.keys().first(), peek);
}

QByteArray SslServer::read(const int length, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peerIp, peek);
}

QByteArray SslServer::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleReadUntil(text, timeout, m_peerHash.keys().first(), peek);
}

QByteArray SslServer::readUntil(const QByteArray &text, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peerIp, peek);
}

// private
bool SslServer::configureSsl() {
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
    m_sslServer->setSslConfiguration(sslConfiguration);
    return true;
}

void SslServer::handleNewConnection() {
    while (m_sslServer->hasPendingConnections()) {
        auto *peer = qobject_cast<QSslSocket *>(m_sslServer->nextPendingConnection());
        if (peer == nullptr) continue;
        const QString peerIp = peer->peerAddress().toString() + ":" + QString::number(peer->peerPort());
        peer->setProperty("peerIp", peerIp);
        connect(peer, &QSslSocket::readyRead, this, [this, peer] { handleReadyRead(peer); });
        connect(peer, &QSslSocket::stateChanged, this, [this, peer](QAbstractSocket::SocketState state) { handleStateChanged(peer, state); });
        connect(peer, &QSslSocket::errorOccurred, this, [this, peer] { handleError(peer); });
        handleStateChanged(peer, peer->state());
    }
}

void SslServer::handleServerError() {
    if (m_sslServer->serverError() == QAbstractSocket::SocketTimeoutError) return;
    if (m_sslServer->isListening()) m_sslServer->close();
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), m_sslServer->errorString());
}

void SslServer::handleStateChanged(QSslSocket *sslServerPeer, const QAbstractSocket::SocketState state) {
    const QString peerIp = sslServerPeer->property("peerIp").toString();
    QString message{};
    switch (state) {
        case QAbstractSocket::UnconnectedState: {
            const auto peer = m_peerHash.constFind(peerIp);
            if (peer == m_peerHash.constEnd() || peer.value() != sslServerPeer) {
                sslServerPeer->deleteLater();
                return;
            }
            m_peerHash.remove(peerIp);
            delete m_bufferHash.take(peerIp);
            sslServerPeer->deleteLater();
            message = QString("lost connection from %1").arg(peerIp);
        }
        break;
        case QAbstractSocket::ConnectedState:
            m_peerHash.insert(peerIp, sslServerPeer);
            m_bufferHash.insert(peerIp, new RingBuffer(m_portConfig["bufferSize"].toInt()));
            message = QString("accepts encrypted connection from %1").arg(peerIp);
            break;
        case QAbstractSocket::ClosingState:
            message = QString("disconnecting from %1").arg(peerIp);
            break;
        default: return;
    }
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void SslServer::handleReadyRead(QSslSocket *sslServerPeer) {
    const QString peerIp = sslServerPeer->property("peerIp").toString();
    const auto rxData = sslServerPeer->readAll();
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    if (buffer == nullptr) return;
    if (buffer->write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), "buffer overflow");
        sslServerPeer->disconnectFromHost();
        return;
    }
    handleLog(LogLevel::Receive, rxData, sslServerPeer);
}

void SslServer::handleError(const QSslSocket *sslServerPeer) {
    if (sslServerPeer == nullptr || sslServerPeer->error() == QAbstractSocket::SocketTimeoutError) return;
    const auto cachedPeerIp = sslServerPeer->property("peerIp").toString();
    const QString peerIp = cachedPeerIp.isEmpty() ? sslServerPeer->peerAddress().toString() + ":" + QString::number(sslServerPeer->peerPort()) : cachedPeerIp;
    emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), QString("error: %1").arg(sslServerPeer->errorString()));
}

bool SslServer::handleWrite(const QByteArray &f_txData, const QString &peerIp) {
    if (m_sslServer == nullptr || !m_sslServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    if (peerIp.isEmpty()) {
        for (auto *peer: m_peerHash) {
            peer->write(f_txData);
            handleLog(LogLevel::Transmit, f_txData, peer);
        }
    } else {
        const auto peer = m_peerHash.constFind(peerIp);
        if (peer == m_peerHash.constEnd()) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
            return false;
        }
        peer.value()->write(f_txData);
        handleLog(LogLevel::Transmit, f_txData, peer.value());
    }
    return true;
}

QByteArray SslServer::handleRead(const int length, const int timeout, const QString &peerIp, const bool peek) {
    if (m_sslServer == nullptr || !m_sslServer->isListening()) {
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
        peer->waitForReadyRead(10);
        buffer = m_bufferHash.value(peerIp, nullptr);
        peer = m_peerHash.value(peerIp, nullptr);
        if (buffer == nullptr || peer == nullptr) return {};
    }
    return buffer->read(length, peek);
}

QByteArray SslServer::handleReadUntil(const QByteArray &text, const int timeout, const QString &peerIp, const bool peek) {
    if (m_sslServer == nullptr || !m_sslServer->isListening()) {
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
        peer->waitForReadyRead(10);
        buffer = m_bufferHash.value(peerIp, nullptr);
        peer = m_peerHash.value(peerIp, nullptr);
        if (buffer == nullptr || peer == nullptr) return {};
        data = buffer->readUntil(text, peek);
    }
    return data;
}

void SslServer::handleLog(const int type, const QByteArray &data, const QSslSocket *sslServerPeer) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    const QString peerIp = sslServerPeer->property("peerIp").toString();
    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    }
}

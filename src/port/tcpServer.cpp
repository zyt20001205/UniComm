#include "port/tcpServer.h"

#include <QDeadlineTimer>
#include <QScopedValueRollback>
#include <QTcpServer>
#include <QTcpSocket>

#include "globals.h"
#include "port/module/ringBuffer.h"
#include "util/uniCast.h"

TcpServer::TcpServer(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig) {
}

TcpServer::~TcpServer() {
    close();
}

int TcpServer::type() {
    return PortType::TcpServer;
}

QJsonObject TcpServer::config() {
    return m_portConfig;
}

QVariantHash TcpServer::info() {
    QVariantHash infoHash = {
        {"status", m_tcpServer && m_tcpServer->isListening() ? "opened" : "closed"},
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

bool TcpServer::open() {
    // status check
    if (m_tcpServer == nullptr) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::handleNewConnection);
        connect(m_tcpServer, &QTcpServer::acceptError, this, &TcpServer::handleServerError);
    }
    if (m_tcpServer->isListening()) return true;
    // open port
    // m_tcpServer->setMaxPendingConnections();
    if (m_tcpServer->listen(QHostAddress(m_portConfig["localHost"].toString()), m_portConfig["localPort"].toInt())) {
        emit refreshPort(m_portConfig["portName"].toString(), true);
        emit appendLog(LogLevel::Info,
                       QString("[%1]").arg(m_portConfig["portName"].toString()),
                       QString("started on %1:%2").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt())));
        return true;
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("open failed: %1").arg(m_tcpServer->errorString()));
    return false;
}

void TcpServer::close() {
    // status check
    if (m_tcpServer == nullptr) return;
    // port close
    m_tcpServer->close();
    for (QTcpSocket *tcpServerPeer: m_peerHash) {
        if (tcpServerPeer) {
            tcpServerPeer->disconnectFromHost();
            if (tcpServerPeer->state() != QAbstractSocket::UnconnectedState) {
                tcpServerPeer->waitForDisconnected(1000);
            }
            tcpServerPeer->deleteLater();
        }
    }
    m_peerHash.clear();
    qDeleteAll(m_bufferHash);
    m_bufferHash.clear();
    clear();
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void TcpServer::disconnectPeer(const QString &peerIp) {
    const auto peer = m_peerHash.constFind(peerIp);
    if (peer == m_peerHash.constEnd()) return;
    peer.value()->disconnectFromHost();
}

void TcpServer::clear() {
    for (RingBuffer *buffer: m_bufferHash) {
        buffer->clear();
    }
}

bool TcpServer::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    // call handle write
    return handleWrite(f_txData);
}

bool TcpServer::write(const QByteArray &txData, const QString &peerIp, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    // call handle write
    return handleWrite(f_txData, peerIp);
}

QByteArray TcpServer::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleRead(length, timeout, m_peerHash.keys().first(), peek);
}

QByteArray TcpServer::read(const int length, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peerIp, peek);
}

QByteArray TcpServer::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (m_peerHash.isEmpty()) return {};
    return handleReadUntil(text, timeout, m_peerHash.keys().first(), peek);
}

QByteArray TcpServer::readUntil(const QByteArray &text, const int timeout, const QString &peerIp, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peerIp, peek);
}

// private
void TcpServer::handleNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *tcpServerPeer = m_tcpServer->nextPendingConnection();
        handleConnected(tcpServerPeer);
        connect(tcpServerPeer, &QTcpSocket::readyRead, this, [this, tcpServerPeer] { handleReadyRead(tcpServerPeer); });
        connect(tcpServerPeer, &QTcpSocket::disconnected, this, [this, tcpServerPeer] { handleDisconnected(tcpServerPeer); });
        connect(tcpServerPeer, &QTcpSocket::errorOccurred, this, [this, tcpServerPeer] { handleError(tcpServerPeer); });
    }
}

void TcpServer::handleServerError() {
    if (m_tcpServer->serverError() == QAbstractSocket::SocketTimeoutError) return;
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_tcpServer->errorString()));
}

void TcpServer::handleConnected(QTcpSocket *tcpServerPeer) {
    const QString peerIp = tcpServerPeer->peerAddress().toString() + ":" + QString::number(tcpServerPeer->peerPort());
    m_peerHash.insert(peerIp, tcpServerPeer);
    m_bufferHash.insert(peerIp, new RingBuffer(m_portConfig["bufferSize"].toInt()));
    emit connected(peerIp);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("accepts connection from %1").arg(peerIp));
}

void TcpServer::handleDisconnected(QTcpSocket *tcpServerPeer) {
    const QString peerIp = tcpServerPeer->peerAddress().toString() + ":" + QString::number(tcpServerPeer->peerPort());
    m_peerHash.remove(peerIp);
    tcpServerPeer->deleteLater();
    delete m_bufferHash[peerIp];
    m_bufferHash.remove(peerIp);
    emit disconnected(peerIp);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("lost connection from %1").arg(peerIp));
}

void TcpServer::handleReadyRead(QTcpSocket *tcpServerPeer) {
    const QString peerIp = tcpServerPeer->peerAddress().toString() + ":" + QString::number(tcpServerPeer->peerPort());
    const auto rxData = tcpServerPeer->readAll();
    auto *buffer = m_bufferHash.value(peerIp, nullptr);
    if (buffer == nullptr) return;
    if (buffer->write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), "buffer overflow");
        close();
        return;
    }
    emit readyRead(peerIp);
    handleLog(LogLevel::Receive, rxData, tcpServerPeer);
}

void TcpServer::handleError(const QTcpSocket *tcpServerPeer) {
    if (tcpServerPeer->error() == QAbstractSocket::SocketTimeoutError) return;
    // if (tcpServerPeer->isOpen()) {
    //     tcpServerPeer->close();
    // }
    const QString peerIp = tcpServerPeer->peerAddress().toString() + ":" + QString::number(tcpServerPeer->peerPort());
    emit appendLog(LogLevel::Error, QString("[%1]").arg(peerIp), QString("error: %1").arg(tcpServerPeer->errorString()));
}

bool TcpServer::handleWrite(const QByteArray &f_txData, const QString &peerIp) {
    // check port status
    if (m_tcpServer == nullptr || !m_tcpServer->isListening()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    if (peerIp.isEmpty()) {
        for (QTcpSocket *tcpServerPeer: m_peerHash) {
            tcpServerPeer->write(f_txData);
            handleLog(LogLevel::Transmit, f_txData, tcpServerPeer);
        }
    } else {
        const auto peer = m_peerHash.constFind(peerIp);
        if (peer == m_peerHash.constEnd()) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "peer not found");
            return false;
        }
        QTcpSocket *tcpServerPeer = peer.value();
        tcpServerPeer->write(f_txData);
        handleLog(LogLevel::Transmit, f_txData, tcpServerPeer);
    }
    return true;
}

QByteArray TcpServer::handleRead(const int length, const int timeout, const QString &peerIp, const bool peek) {
    // check port status
    if (m_tcpServer == nullptr || !m_tcpServer->isListening()) {
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

QByteArray TcpServer::handleReadUntil(const QByteArray &text, const int timeout, const QString &peerIp, const bool peek) {
    // check port status
    if (m_tcpServer == nullptr || !m_tcpServer->isListening()) {
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

void TcpServer::handleLog(const int type, const QByteArray &data, const QTcpSocket *tcpServerPeer) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    const QString peerIp = tcpServerPeer->peerAddress().toString() + ":" + QString::number(tcpServerPeer->peerPort());
    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3]").arg(m_portConfig["localHost"].toString(), QString::number(m_portConfig["localPort"].toInt()), peerIp), message);
    }
}

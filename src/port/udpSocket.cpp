#include "port/udpSocket.h"

#include <QDeadlineTimer>
#include <QScopedValueRollback>
#include <QUdpSocket>

#include "globals.h"
#include "util/uniCast.h"

// public
UdpSocket::UdpSocket(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

UdpSocket::~UdpSocket() {
    close();
}

int UdpSocket::type() {
    return PortType::UdpSocket;
}

QJsonObject UdpSocket::config() {
    return m_portConfig;
}

QVariantHash UdpSocket::info() {
    return {
        {"status", m_udpSocket && m_udpSocket->state() == QAbstractSocket::ConnectedState ? "connected" : "disconnected"},
        {"localHost", m_portConfig["localHost"].toString()},
        {"localPort", QString::number(m_udpSocket ? m_udpSocket->localPort() : 0)},
        {"remoteHost", m_portConfig["remoteHost"].toString()},
        {"remotePort", QString::number(m_portConfig["remotePort"].toInt())},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool UdpSocket::open() {
    // status check
    if (m_udpSocket == nullptr) {
        m_udpSocket = new QUdpSocket(this);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpSocket::handleReadyRead);
        connect(m_udpSocket, &QUdpSocket::errorOccurred, this, &UdpSocket::handleError);
    }
    if (m_udpSocket->state() != QAbstractSocket::UnconnectedState) return true;
    // open port
    m_buffer.clear();
    if (!m_udpSocket->bind(QHostAddress(m_portConfig["localHost"].toString()), m_portConfig["localPort"].toInt())) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("open failed: %1").arg(m_udpSocket->errorString()));
        return false;
    }
    m_udpSocket->connectToHost(m_portConfig["remoteHost"].toString(), m_portConfig["remotePort"].toInt());
    emit refreshPort(m_portConfig["portName"].toString(), true);
    emit appendLog(LogLevel::Info,
                   QString("[%1]").arg(m_portConfig["portName"].toString()),
                   QString("opened: %1:%2->%3:%4")
                   .arg(m_portConfig["localHost"].toString(),
                        QString::number(m_udpSocket->localPort()),
                        m_portConfig["remoteHost"].toString(),
                        QString::number(m_portConfig["remotePort"].toInt())));
    return true;
}

void UdpSocket::close() {
    // status check
    if (m_udpSocket == nullptr) return;
    if (m_udpSocket->isOpen()) {
        m_udpSocket->close();
    }
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void UdpSocket::clear() {
    m_buffer.clear();
}

bool UdpSocket::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
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

QByteArray UdpSocket::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray UdpSocket::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
void UdpSocket::handleReadyRead() {
    const auto rxData = m_udpSocket->readAll();
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
    }
    handleLog(LogLevel::Receive, rxData);
}

void UdpSocket::handleError() {
    if (m_udpSocket->error() == QAbstractSocket::SocketTimeoutError) return;
    if (m_udpSocket->isOpen()) {
        m_udpSocket->close();
    }
    emit refreshPort(m_portConfig["portName"].toString(), false);
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_udpSocket->errorString()));
}

bool UdpSocket::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_udpSocket == nullptr || !m_udpSocket->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    m_udpSocket->write(f_txData);
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray UdpSocket::handleRead(const int length, const int timeout, const bool peek) {
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < minimum) {
        if (m_udpSocket == nullptr || !m_udpSocket->isOpen() || deadline.hasExpired()) break;
        m_udpSocket->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray UdpSocket::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (m_udpSocket == nullptr || !m_udpSocket->isOpen() || deadline.hasExpired()) break;
        m_udpSocket->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void UdpSocket::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3:%4]").arg(m_portConfig["localHost"].toString(), QString::number(m_udpSocket->localPort()), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3:%4] ").arg(m_portConfig["localHost"].toString(), QString::number(m_udpSocket->localPort()), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    }
}

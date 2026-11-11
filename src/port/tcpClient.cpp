#include "port/tcpClient.h"

#include <QDeadlineTimer>
#include <QScopedValueRollback>
#include <QTcpSocket>

#include "globals.h"
#include "util/uniCast.h"

// public
TcpClient::TcpClient(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

TcpClient::~TcpClient() {
    close();
}

int TcpClient::type() {
    return PortType::TcpClient;
}

QJsonObject TcpClient::config() {
    return m_portConfig;
}

QVariantHash TcpClient::info() {
    QString status{};
    if (m_tcpClient == nullptr) {
        status = "unconnected";
    } else {
        switch (m_tcpClient->state()) {
            case QAbstractSocket::UnconnectedState: status = "unconnected";
                break;
            case QAbstractSocket::HostLookupState: status = "looking up host";
                break;
            case QAbstractSocket::ConnectingState: status = "connecting";
                break;
            case QAbstractSocket::ConnectedState: status = "connected";
                break;
            case QAbstractSocket::ClosingState: status = "closing";
                break;
            case QAbstractSocket::BoundState: status = "bound to local address";
                break;
            default: status = "unknown";
        }
    }
    return {
        {"status", status},
        {"localHost", m_tcpClientLocalHost},
        {"localPort", QString::number(m_tcpClientLocalPort)},
        {"remoteHost", m_portConfig["remoteHost"].toString()},
        {"remotePort", QString::number(m_portConfig["remotePort"].toInt())},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool TcpClient::open() {
    if (m_tcpClient == nullptr) {
        m_tcpClient = new QTcpSocket(this);
        connect(m_tcpClient, &QTcpSocket::stateChanged, this, &TcpClient::handleStateChanged);
        connect(m_tcpClient, &QTcpSocket::readyRead, this, &TcpClient::handleReadyRead);
        connect(m_tcpClient, &QTcpSocket::errorOccurred, this, &TcpClient::handleError);
    }
    if (m_tcpClient->state() == QAbstractSocket::ConnectedState) return true;
    if (m_tcpClient->state() == QAbstractSocket::ClosingState) m_tcpClient->abort();
    if (m_tcpClient->state() == QAbstractSocket::UnconnectedState) {
        m_buffer.clear();
        m_tcpClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_tcpClient->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        m_tcpClient->connectToHost(m_portConfig["remoteHost"].toString(), m_portConfig["remotePort"].toInt());
    }
    if (!m_tcpClient->waitForConnected()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
        return false;
    }
    return true;
}

void TcpClient::close() {
    if (m_tcpClient == nullptr) return;
    switch (m_tcpClient->state()) {
        case QAbstractSocket::ConnectedState:
            m_tcpClient->disconnectFromHost();
            break;
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::HostLookupState:
            m_tcpClient->abort();
            break;
        default: break;
    }
}

void TcpClient::clear() {
    m_buffer.clear();
}

bool TcpClient::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
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

QByteArray TcpClient::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray TcpClient::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
void TcpClient::handleStateChanged(const QAbstractSocket::SocketState state) {
    const auto remote = QString("%1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt()));
    QString message{};
    switch (state) {
        case QAbstractSocket::UnconnectedState:
            message = QString("unconnected: %1").arg(remote);
            break;
        case QAbstractSocket::HostLookupState:
            message = QString("looking up host for %1").arg(remote);
            break;
        case QAbstractSocket::ConnectingState:
            message = QString("connecting to %1").arg(remote);
            break;
        case QAbstractSocket::ConnectedState:
            m_tcpClientLocalHost = m_tcpClient->localAddress().toString();
            m_tcpClientLocalPort = m_tcpClient->localPort();
            message = QString("connected to %1").arg(remote);
            break;
        case QAbstractSocket::ClosingState:
            message = QString("disconnecting from %1").arg(remote);
            break;
        default: break;
    }
    emit refreshPort(m_portConfig["portName"].toString(), state == QAbstractSocket::ConnectedState);
    if (!message.isEmpty()) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void TcpClient::handleReadyRead() {
    const auto rxData = m_tcpClient->readAll();
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
    }
    handleLog(LogLevel::Receive, rxData);
}

void TcpClient::handleError() {
    if (m_tcpClient->error() == QAbstractSocket::SocketTimeoutError || m_tcpClient->error() == QAbstractSocket::RemoteHostClosedError) return;
    if (m_tcpClient->isOpen()) {
        m_tcpClient->close();
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_tcpClient->errorString()));
}

bool TcpClient::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_tcpClient == nullptr || m_tcpClient->state() != QAbstractSocket::ConnectedState) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    m_tcpClient->write(f_txData);
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray TcpClient::handleRead(const int length, const int timeout, const bool peek) {
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < minimum) {
        if (m_tcpClient == nullptr || m_tcpClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_tcpClient->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray TcpClient::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (m_tcpClient == nullptr || m_tcpClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_tcpClient->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void TcpClient::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3:%4]").arg(m_tcpClientLocalHost, QString::number(m_tcpClientLocalPort), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3:%4]").arg(m_tcpClientLocalHost, QString::number(m_tcpClientLocalPort), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    }
}

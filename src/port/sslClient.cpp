#include "port/sslClient.h"

#include <QDeadlineTimer>
#include <QScopedValueRollback>
#include <QSslSocket>

#include "globals.h"
#include "util/uniCast.h"

// public
SslClient::SslClient(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

SslClient::~SslClient() {
    close();
}

int SslClient::type() {
    return PortType::SslClient;
}

QJsonObject SslClient::config() {
    return m_portConfig;
}

QVariantHash SslClient::info() {
    QString status{};
    if (m_sslClient == nullptr) {
        status = "unconnected";
    } else {
        switch (m_sslClient->state()) {
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
        {"localHost", m_sslClientLocalHost},
        {"localPort", QString::number(m_sslClientLocalPort)},
        {"remoteHost", m_portConfig["remoteHost"].toString()},
        {"remotePort", QString::number(m_portConfig["remotePort"].toInt())},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool SslClient::open() {
    if (m_sslClient == nullptr) {
        m_sslClient = new QSslSocket(this);
        // TODO: This is not safe!!! test only
        m_sslClient->setPeerVerifyMode(QSslSocket::VerifyNone);
        connect(m_sslClient, &QSslSocket::stateChanged, this, &SslClient::handleStateChanged);
        connect(m_sslClient, &QSslSocket::encrypted, this, [this] { handleStateChanged(m_sslClient->state()); });
        connect(m_sslClient, &QSslSocket::readyRead, this, &SslClient::handleReadyRead);
        connect(m_sslClient, &QSslSocket::errorOccurred, this, &SslClient::handleError);
    }
    if (m_sslClient->state() == QAbstractSocket::ConnectedState && m_sslClient->isEncrypted()) return true;
    if (m_sslClient->state() == QAbstractSocket::ClosingState) m_sslClient->abort();
    if (m_sslClient->state() == QAbstractSocket::UnconnectedState) {
        m_buffer.clear();
        m_sslClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_sslClient->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        m_sslClient->connectToHostEncrypted(m_portConfig["remoteHost"].toString(), m_portConfig["remotePort"].toInt());
    }
    if (!m_sslClient->isEncrypted() && !m_sslClient->waitForEncrypted()) {
        m_sslClient->abort();
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
        return false;
    }
    return true;
}

void SslClient::close() {
    if (m_sslClient == nullptr) return;
    switch (m_sslClient->state()) {
        case QAbstractSocket::ConnectedState:
            m_sslClient->disconnectFromHost();
            break;
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::HostLookupState:
            m_sslClient->abort();
            break;
        default: break;
    }
}

void SslClient::clear() {
    m_buffer.clear();
}

bool SslClient::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
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

QByteArray SslClient::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray SslClient::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
void SslClient::handleStateChanged(const QAbstractSocket::SocketState state) {
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
            m_sslClientLocalHost = m_sslClient->localAddress().toString();
            m_sslClientLocalPort = m_sslClient->localPort();
            message = m_sslClient->isEncrypted()
                          ? QString("encrypted connection established with %1").arg(remote)
                          : QString("connected to %1").arg(remote);
            break;
        case QAbstractSocket::ClosingState:
            message = QString("disconnecting from %1").arg(remote);
            break;
        default: break;
    }
    emit refreshPort(m_portConfig["portName"].toString(), state == QAbstractSocket::ConnectedState && m_sslClient->isEncrypted());
    if (!message.isEmpty()) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void SslClient::handleReadyRead() {
    const auto rxData = m_sslClient->readAll();
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
    }
    handleLog(LogLevel::Receive, rxData);
}

void SslClient::handleError() {
    if (m_sslClient->error() == QAbstractSocket::SocketTimeoutError) return;
    if (m_sslClient->error() == QAbstractSocket::RemoteHostClosedError) {
        m_sslClient->disconnectFromHost();
        return;
    }
    if (m_sslClient->isOpen()) {
        m_sslClient->close();
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_sslClient->errorString()));
}

bool SslClient::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_sslClient == nullptr || m_sslClient->state() != QAbstractSocket::ConnectedState || !m_sslClient->isEncrypted()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    m_sslClient->write(f_txData);
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray SslClient::handleRead(const int length, const int timeout, const bool peek) {
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < minimum) {
        if (m_sslClient == nullptr || m_sslClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_sslClient->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray SslClient::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (m_sslClient == nullptr || m_sslClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_sslClient->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void SslClient::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3:%4]").arg(m_sslClientLocalHost, QString::number(m_sslClientLocalPort), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3:%4]").arg(m_sslClientLocalHost, QString::number(m_sslClientLocalPort), m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())), message);
    }
}

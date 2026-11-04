#include "port/sslClient.h"

#include <QElapsedTimer>
#include <QScopedValueRollback>
#include <QSslSocket>
#include <QTimer>

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
    const auto localHost = m_sslClientLocalHost;
    const auto localPort = QString::number(m_sslClientLocalPort);
    const auto remoteHost = m_portConfig["remoteHost"].toString();
    const auto remotePort = QString::number(m_portConfig["remotePort"].toInt());
    const auto bufferSize = QString::number(m_portConfig["bufferSize"].toInt());
    const auto bufferUsed = QString::number(m_buffer.used());

    const QVariantHash infoHash = {
        {"status", status},
        {"localHost", localHost},
        {"localPort", localPort},
        {"remoteHost", remoteHost},
        {"remotePort", remotePort},
        {"bufferSize", bufferSize},
        {"bufferUsed", bufferUsed}
    };
    return infoHash;
}

bool SslClient::open() {
    // status check
    if (m_sslClient == nullptr) {
        m_sslClient = new QSslSocket(this);
        // TODO: This is not safe!!! test only
        m_sslClient->setPeerVerifyMode(QSslSocket::VerifyNone);
        connect(m_sslClient, &QSslSocket::connected, this, &SslClient::handleConnected);
        connect(m_sslClient, &QSslSocket::encrypted, this, &SslClient::handleEncrypted);
        connect(m_sslClient, &QSslSocket::disconnected, this, &SslClient::handleDisconnected);
        connect(m_sslClient, &QSslSocket::readyRead, this, &SslClient::handleReadyRead);
        connect(m_sslClient, &QSslSocket::errorOccurred, this, &SslClient::handleError);
    }
    if (m_monitorTimer == nullptr) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(16);
        m_monitorTimer->setSingleShot(false);
        connect(m_monitorTimer, &QTimer::timeout, this, &SslClient::handleUpdate);
    }
    if (m_sslClient->state() != QAbstractSocket::UnconnectedState) return true;
    // open port
    m_sslClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_sslClient->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    emit appendLog(LogLevel::Info,
                   QString("[%1]").arg(m_portConfig["portName"].toString()),
                   QString("connecting to %1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
    m_sslClient->connectToHostEncrypted(m_portConfig["remoteHost"].toString(), m_portConfig["remotePort"].toInt());
    if (!m_sslClient->waitForEncrypted()) {
        handleError();
        return false;
    }
    m_buffer.clear();
    m_buffer.resetStatistics();
    m_activeTimer.start();
    const QVariantHash session{
        {"active", true},
        {"capacity", m_portConfig["bufferSize"].toInt()},
        {"lifetime", uni_cast<QLifetime>(qint64{}).value}
    };
    emit refreshPort(m_portConfig["portName"].toString(), session);
    return true;
}

void SslClient::close() {
    // status check
    if (m_sslClient == nullptr) return;
    // port close
    switch (m_sslClient->state()) {
        case QAbstractSocket::ConnectedState: {
            emit appendLog(LogLevel::Info,
                           QString("[%1]").arg(m_portConfig["portName"].toString()),
                           QString("disconnecting from %1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
            m_sslClient->disconnectFromHost();
        }
        break;
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::HostLookupState: {
            emit appendLog(LogLevel::Info,
                           QString("[%1]").arg(m_portConfig["portName"].toString()),
                           QString("connection to %1:%2 cancelled").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
            m_sslClient->abort();
        }
        break;
        default: break;
    }
    if (m_monitorTimer) m_monitorTimer->stop();
    const QVariantHash session{{"active", false}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
}

void SslClient::clear() {
    m_buffer.clear();
}

void SslClient::monitor(const bool enabled) {
    if (m_monitorTimer == nullptr || m_sslClient == nullptr || m_sslClient->state() != QAbstractSocket::ConnectedState) return;
    if (enabled) {
        handleUpdate();
        m_monitorTimer->start();
    } else {
        m_monitorTimer->stop();
    }
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
void SslClient::handleConnected() {
    m_sslClientLocalHost = m_sslClient->localAddress().toString();
    m_sslClientLocalPort = m_sslClient->localPort();
    emit appendLog(LogLevel::Info,
                   QString("[%1]").arg(m_portConfig["portName"].toString()),
                   QString("connected to %1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
}

void SslClient::handleEncrypted() {
    emit appendLog(LogLevel::Info,
                   QString("[%1]").arg(m_portConfig["portName"].toString()),
                   QString("encrypted connection established with %1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
}

void SslClient::handleDisconnected() {
    if (m_monitorTimer) m_monitorTimer->stop();
    clear();
    const QVariantHash session{{"active", false}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Info,
                   QString("[%1]").arg(m_portConfig["portName"].toString()),
                   QString("disconnected from %1:%2").arg(m_portConfig["remoteHost"].toString(), QString::number(m_portConfig["remotePort"].toInt())));
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
    if (m_sslClient->isOpen()) {
        m_sslClient->close();
    }
    if (m_monitorTimer) m_monitorTimer->stop();
    const QVariantHash session{{"active", false}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_sslClient->errorString()));
}

bool SslClient::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_sslClient == nullptr || !m_sslClient->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    m_sslClient->write(f_txData);
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray SslClient::handleRead(const int length, const int timeout, const bool peek) {
    // check port status
    if (m_sslClient == nullptr || !m_sslClient->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < length) {
        if (deadline.hasExpired()) break;
        m_sslClient->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray SslClient::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    // check port status
    if (m_sslClient == nullptr || !m_sslClient->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (deadline.hasExpired()) break;
        m_sslClient->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void SslClient::handleUpdate() {
    const auto statistics = m_buffer.statistics();
    const auto &session = QVariantHash{
        {"used", statistics.used},
        {"lifetime", uni_cast<QLifetime>(m_activeTimer.elapsed()).value},
        {"readCount", statistics.readCount},
        {"readBytes", statistics.readBytes},
        {"writeCount", statistics.writeCount},
        {"writeBytes", statistics.writeBytes}
    };
    emit refreshPort(m_portConfig["portName"].toString(), session);
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

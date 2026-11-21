#include "port/sslClient.h"

#include <QDeadlineTimer>
#include <QFile>
#include <QScopedValueRollback>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QUrl>

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

PortType::Value SslClient::type() {
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
        {"secure", m_portConfig["secure"].toBool()},
        {"caCertificate", m_portConfig["caCertificate"].toString()},
        {"certificate", m_portConfig["certificate"].toString()},
        {"privateKey", m_portConfig["privateKey"].toString()},
        {"hasPrivateKeyPassword", !m_portConfig["privateKeyPassword"].toString().isEmpty()},
        {"protocol", m_sslClient && m_sslClient->isEncrypted() ? m_sslClient->sessionCipher().protocolString() : QString{}},
        {"cipher", m_sslClient && m_sslClient->isEncrypted() ? m_sslClient->sessionCipher().name() : QString{}},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool SslClient::open() {
    if (m_sslClient == nullptr) {
        m_sslClient = new QSslSocket(this);
        connect(m_sslClient, &QSslSocket::stateChanged, this, &SslClient::handleStateChanged);
        connect(m_sslClient, &QSslSocket::encrypted, this, [this] { handleStateChanged(m_sslClient->state()); });
        connect(m_sslClient, &QSslSocket::readyRead, this, &SslClient::handleReadyRead);
        connect(m_sslClient, &QSslSocket::errorOccurred, this, &SslClient::handleError);
        connect(m_sslClient, &QSslSocket::sslErrors, this, &SslClient::handleErrors);
    }
    if (m_sslClient->state() == QAbstractSocket::ConnectedState && m_sslClient->isEncrypted()) return true;
    if (m_sslClient->state() == QAbstractSocket::ClosingState) m_sslClient->abort();
    if (m_sslClient->state() == QAbstractSocket::UnconnectedState) {
        if (const auto exception = configure(); !exception.isEmpty()) {
            emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed: " + exception);
            emit refreshPort(m_portConfig["portName"].toString(), false);
            return false;
        }
        m_buffer.clear();
        m_sslClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_sslClient->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        m_sslClient->connectToHostEncrypted(m_portConfig["remoteHost"].toString(), m_portConfig["remotePort"].toInt());
    }
    if (!m_sslClient->isEncrypted() && !m_sslClient->waitForEncrypted()) {
        m_sslClient->abort();
        emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
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
QString SslClient::configure() {
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    configuration.setLocalCertificateChain({});
    configuration.setPrivateKey({});
    const bool secure = m_portConfig["secure"].toBool();
    configuration.setPeerVerifyMode(secure ? QSslSocket::VerifyPeer : QSslSocket::VerifyNone);
    if (secure) {
        const auto caCertificate = m_portConfig["caCertificate"].toString();
        if (!caCertificate.isEmpty()) {
            QFile file(QUrl(caCertificate).toLocalFile());
            if (!file.open(QIODevice::ReadOnly)) return "cannot open CA certificate: " + file.errorString();
            const auto certificates = QSslCertificate::fromDevice(&file, QSsl::Pem);
            if (certificates.isEmpty()) return "CA certificate invalid";
            configuration.addCaCertificates(certificates);
        }

        const auto certificate = m_portConfig["certificate"].toString();
        const auto privateKey = m_portConfig["privateKey"].toString();
        const auto password = m_portConfig["privateKeyPassword"].toString().toUtf8();
        if (certificate.isEmpty() != privateKey.isEmpty()) return "client certificate and private key must be provided together";
        if (certificate.isEmpty() && !password.isEmpty()) return "private key password requires a client certificate and private key";
        if (!certificate.isEmpty()) {
            QFile certificateFile(QUrl(certificate).toLocalFile());
            if (!certificateFile.open(QIODevice::ReadOnly)) return "cannot open client certificate: " + certificateFile.errorString();
            const auto certificates = QSslCertificate::fromDevice(&certificateFile, QSsl::Pem);
            if (certificates.isEmpty()) return "client certificate invalid";

            QFile privateKeyFile(QUrl(privateKey).toLocalFile());
            if (!privateKeyFile.open(QIODevice::ReadOnly)) return "cannot open private key: " + privateKeyFile.errorString();
            const QSslKey key(&privateKeyFile, certificates.first().publicKey().algorithm(), QSsl::Pem, QSsl::PrivateKey, password);
            if (key.isNull()) return "private key invalid or password incorrect";
            configuration.setLocalCertificateChain(certificates);
            configuration.setPrivateKey(key);
        }
    }
    m_sslClient->setSslConfiguration(configuration);
    return {};
}

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
    if (!message.isEmpty()) emit appendLog(LogType::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void SslClient::handleReadyRead() {
    const auto rxData = m_sslClient->readAll();
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
    }
    handleLog(LogType::Receive, rxData);
}

void SslClient::handleError() {
    if (m_sslClient->error() == QAbstractSocket::SocketTimeoutError || m_sslClient->error() == QAbstractSocket::RemoteHostClosedError) return;
    if (m_sslClient->error() == QAbstractSocket::SslHandshakeFailedError && !m_sslClient->sslHandshakeErrors().isEmpty()) return;
    emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_sslClient->errorString()));
}

void SslClient::handleErrors(const QList<QSslError> &errors) {
    for (const auto &error: errors) emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), error.errorString());
}

bool SslClient::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_sslClient == nullptr || m_sslClient->state() != QAbstractSocket::ConnectedState || !m_sslClient->isEncrypted()) {
        emit appendLog(LogType::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    m_sslClient->write(f_txData);
    handleLog(LogType::Transmit, f_txData);
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

void SslClient::handleLog(const LogType::Value type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogType::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3:%4]").arg(m_sslClientLocalHost, QString::number(m_sslClientLocalPort), m_portConfig["remoteHost"].toString(),
                                                                QString::number(m_portConfig["remotePort"].toInt())), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3:%4]").arg(m_sslClientLocalHost, QString::number(m_sslClientLocalPort), m_portConfig["remoteHost"].toString(),
                                                                QString::number(m_portConfig["remotePort"].toInt())), message);
    }
}

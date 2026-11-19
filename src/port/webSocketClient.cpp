#include "port/webSocketClient.h"

#include <QDeadlineTimer>
#include <QFile>
#include <QScopedValueRollback>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

#include "globals.h"
#include "util/uniCast.h"

// public
WebSocketClient::WebSocketClient(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
    m_url = QUrl((portConfig["secure"].toBool() ? "wss://" : "ws://") + portConfig["remoteHost"].toString());
    m_url.setPort(portConfig["remotePort"].toInt());
}

WebSocketClient::~WebSocketClient() {
    close();
}

int WebSocketClient::type() {
    return PortType::WebSocketClient;
}

QJsonObject WebSocketClient::config() {
    return m_portConfig;
}

QVariantHash WebSocketClient::info() {
    QString status{};
    if (m_webSocketClient == nullptr) {
        status = "unconnected";
    } else {
        switch (m_webSocketClient->state()) {
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
        {"localHost", m_localHost},
        {"localPort", QString::number(m_localPort)},
        {"remoteHost", m_url.host()},
        {"remotePort", QString::number(m_url.port())},
        {"secure", m_portConfig["secure"].toBool()},
        {"caCertificate", m_portConfig["caCertificate"].toString()},
        {"certificate", m_portConfig["certificate"].toString()},
        {"privateKey", m_portConfig["privateKey"].toString()},
        {"hasPrivateKeyPassword", !m_portConfig["privateKeyPassword"].toString().isEmpty()},
        {"protocol", m_webSocketClient && m_portConfig["secure"].toBool() && m_webSocketClient->state() == QAbstractSocket::ConnectedState ? m_webSocketClient->sslConfiguration().sessionCipher().protocolString() : QString{}},
        {"cipher", m_webSocketClient && m_portConfig["secure"].toBool() && m_webSocketClient->state() == QAbstractSocket::ConnectedState ? m_webSocketClient->sslConfiguration().sessionCipher().name() : QString{}},
        {"closeCode", m_webSocketClient ? static_cast<int>(m_webSocketClient->closeCode()) : 0},
        {"closeReason", m_webSocketClient ? m_webSocketClient->closeReason() : QString()},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool WebSocketClient::open() {
    if (m_webSocketClient == nullptr) {
        m_webSocketClient = new UWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
        connect(m_webSocketClient, &QWebSocket::stateChanged, this, &WebSocketClient::handleStateChanged);
        connect(m_webSocketClient, &QWebSocket::binaryMessageReceived, this, [this](const QByteArray &message) { handleMessage(message); });
        connect(m_webSocketClient, &QWebSocket::textMessageReceived, this, [this](const QString &message) { handleMessage(message.toUtf8()); });
        connect(m_webSocketClient, &QWebSocket::errorOccurred, this, &WebSocketClient::handleError);
        connect(m_webSocketClient, &QWebSocket::sslErrors, this, &WebSocketClient::handleErrors);
    }
    if (m_webSocketClient->state() == QAbstractSocket::ConnectedState) return true;
    if (m_webSocketClient->state() == QAbstractSocket::ClosingState) m_webSocketClient->abort();
    if (m_webSocketClient->state() == QAbstractSocket::UnconnectedState) {
        m_sslErrorsHandled = false;
        if (const auto exception = configure(); !exception.isEmpty()) {
            emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed: " + exception);
            emit refreshPort(m_portConfig["portName"].toString(), false);
            return false;
        }
        m_buffer.clear();
        m_webSocketClient->open(m_url);
    }
    if (!m_webSocketClient->waitForConnected()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
        return false;
    }

    return true;
}

void WebSocketClient::close() {
    if (m_webSocketClient == nullptr) return;

    switch (m_webSocketClient->state()) {
        case QAbstractSocket::ConnectedState:
            m_webSocketClient->close();
            break;
        case QAbstractSocket::ConnectingState:
        case QAbstractSocket::HostLookupState:
            m_webSocketClient->abort();
            break;
        default:
            break;
    }
}

void WebSocketClient::clear() {
    m_buffer.clear();
}

bool WebSocketClient::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    return write(txData, WebSocketMessageType::Binary, logFormat, txSuffix);
}

bool WebSocketClient::write(const QByteArray &txData, const int messageType, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData, messageType);
}

QByteArray WebSocketClient::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray WebSocketClient::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
QString WebSocketClient::configure() {
    if (!m_portConfig["secure"].toBool()) return {};

    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    configuration.setLocalCertificateChain({});
    configuration.setPrivateKey({});
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);

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
    m_webSocketClient->setSslConfiguration(configuration);
    return {};
}

void WebSocketClient::handleStateChanged(const QAbstractSocket::SocketState state) {
    const auto url = m_url.toString();
    QString message{};
    switch (state) {
        case QAbstractSocket::UnconnectedState: {
            message = QString("unconnected: %1 (close code %2)").arg(url).arg(static_cast<int>(m_webSocketClient->closeCode()));
            const auto reason = m_webSocketClient->closeReason();
            if (!reason.isEmpty()) message += ": " + reason;
        }
        break;
        case QAbstractSocket::HostLookupState:
            message = QString("looking up host for %1").arg(url);
            break;
        case QAbstractSocket::ConnectingState:
            message = QString("connecting to %1").arg(url);
            break;
        case QAbstractSocket::ConnectedState:
            m_localHost = m_webSocketClient->localAddress().toString();
            m_localPort = m_webSocketClient->localPort();
            message = QString("connected to %1").arg(url);
            break;
        case QAbstractSocket::ClosingState:
            message = QString("disconnecting from %1").arg(url);
            break;
        default: break;
    }
    emit refreshPort(m_portConfig["portName"].toString(), state == QAbstractSocket::ConnectedState);
    if (!message.isEmpty()) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), message);
}

void WebSocketClient::handleMessage(const QByteArray &message) {
    if (m_buffer.write(message) != message.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
        return;
    }
    handleLog(LogLevel::Receive, message);
}

void WebSocketClient::handleError() {
    if (m_webSocketClient->error() == QAbstractSocket::SocketTimeoutError) return;
    if (m_webSocketClient->error() == QAbstractSocket::SslHandshakeFailedError && m_sslErrorsHandled) return;
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), m_webSocketClient->errorString());
}

void WebSocketClient::handleErrors(const QList<QSslError> &errors) {
    m_sslErrorsHandled = !errors.isEmpty();
    for (const auto &error: errors) emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), error.errorString());
}

bool WebSocketClient::handleWrite(const QByteArray &f_txData, const int messageType) {
    if (messageType != WebSocketMessageType::Text && messageType != WebSocketMessageType::Binary) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "invalid message type");
        return false;
    }
    if (messageType == WebSocketMessageType::Text && !f_txData.isValidUtf8()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "invalid UTF-8 text");
        return false;
    }
    if (m_webSocketClient == nullptr || m_webSocketClient->state() != QAbstractSocket::ConnectedState) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    const auto written = messageType == WebSocketMessageType::Text
                             ? m_webSocketClient->sendTextMessage(QString::fromUtf8(f_txData))
                             : m_webSocketClient->sendBinaryMessage(f_txData);
    if (written != f_txData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "write failed");
        return false;
    }
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray WebSocketClient::handleRead(const int length, const int timeout, const bool peek) {
    const int minimum = length == 0 ? 1 : length;
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < minimum) {
        if (m_webSocketClient == nullptr || m_webSocketClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_webSocketClient->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray WebSocketClient::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (m_webSocketClient == nullptr || m_webSocketClient->state() != QAbstractSocket::ConnectedState || deadline.hasExpired()) break;
        m_webSocketClient->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void WebSocketClient::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1:%2 -&gt; %3]").arg(m_localHost, QString::number(m_localPort), m_url.toString()), message);
    } else {
        emit appendLog(type, QString("[%1:%2 &lt;- %3]").arg(m_localHost, QString::number(m_localPort), m_url.toString()), message);
    }
}

// public
bool UWebSocket::waitForConnected(const int msecs) {
    if (state() == QAbstractSocket::ConnectedState) return true;
    if (state() == QAbstractSocket::UnconnectedState) return false;

    QEventLoop loop{};
    QTimer timer{};
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &QWebSocket::connected, &loop, &QEventLoop::quit);
    connect(this, &QWebSocket::errorOccurred, &loop, &QEventLoop::quit);
    connect(this, &QWebSocket::disconnected, &loop, &QEventLoop::quit);
    if (msecs >= 0) timer.start(msecs);
    loop.exec();
    if (state() == QAbstractSocket::ConnectedState) return true;
    abort();
    return false;
}

bool UWebSocket::waitForReadyRead(const int msecs) {
    return waitForReadyRead(this, msecs);
}

bool UWebSocket::waitForReadyRead(QWebSocket *socket, const int msecs) {
    if (socket->state() != QAbstractSocket::ConnectedState) return false;

    QEventLoop loop{};
    QTimer timer{};
    bool ready = false;
    timer.setSingleShot(true);
    connect(socket, &QWebSocket::binaryMessageReceived, &loop, [&] {
        ready = true;
        loop.quit();
    });
    connect(socket, &QWebSocket::textMessageReceived, &loop, [&] {
        ready = true;
        loop.quit();
    });
    connect(socket, &QWebSocket::disconnected, &loop, &QEventLoop::quit);
    connect(socket, &QWebSocket::errorOccurred, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    if (msecs >= 0) timer.start(msecs);
    loop.exec();
    return ready;
}

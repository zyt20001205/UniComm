#include "port/serialPort.h"

#include <QDeadlineTimer>
#include <QScopedValueRollback>
#include <QSerialPort>
#include <QThread>
#include <QTimer>

#include "globals.h"
#include "util/uniCast.h"

// public
SerialPort::SerialPort(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

SerialPort::~SerialPort() {
    close();
}

int SerialPort::type() {
    return PortType::SerialPort;
}

QJsonObject SerialPort::config() {
    return m_portConfig;
}

QVariantHash SerialPort::info() {
    const QString status = m_serialPort && m_serialPort->isOpen() ? "opened" : "closed";
    const auto portName = m_portConfig["portName"].toString();
    const auto baudRate = QString::number(m_portConfig["baudRate"].toInt());
    const auto dataBits = QString::number(m_portConfig["dataBits"].toInt());
    QString parity{};
    switch (m_portConfig["parity"].toInt()) {
        case 0: parity = "no";
            break;
        case 2: parity = "even";
            break;
        case 3: parity = "odd";
            break;
        case 4: parity = "space";
            break;
        case 5: parity = "mark";
            break;
        default: parity = "?";
    }
    QString stopBits{};
    switch (m_portConfig["stopBits"].toInt()) {
        case 1: stopBits = "1";
            break;
        case 3: stopBits = "1.5";
            break;
        case 2: stopBits = "2";
            break;
        default: stopBits = "?";
    }
    const auto bufferSize = QString::number(m_portConfig["bufferSize"].toInt());
    const auto bufferUsed = QString::number(m_buffer.used());

    QVariantHash infoHash = {
        {"status", status},
        {"portName", portName},
        {"baudRate", baudRate},
        {"dataBits", dataBits},
        {"parity", parity},
        {"stopBits", stopBits},
        {"bufferSize", bufferSize},
        {"bufferUsed", bufferUsed}
    };
    return infoHash;
}

bool SerialPort::open() {
    // status check
    if (m_serialPort == nullptr) {
        m_serialPort = new QSerialPort(this);
        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPort::handleReadyRead);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialPort::handleError);
    }
    if (m_monitorTimer == nullptr) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(16);
        m_monitorTimer->setSingleShot(false);
        connect(m_monitorTimer, &QTimer::timeout, this, &SerialPort::handleUpdate);
    }
    if (m_serialPort->isOpen()) return true;
    // port open
    m_serialPort->setPortName(m_portConfig["portName"].toString());
    m_serialPort->setBaudRate(m_portConfig["baudRate"].toInt());
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(m_portConfig["dataBits"].toInt()));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(m_portConfig["parity"].toInt()));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(m_portConfig["stopBits"].toInt()));
    if (m_serialPort->open(QSerialPort::ReadWrite)) {
        m_buffer.clear();
        m_buffer.resetStatistics();
        m_activeTimer.start();
        const auto &session = QVariantHash{
            {"active", true},
            {"capacity", m_portConfig["bufferSize"].toInt()},
            {"lifetime", uni_cast<QLifetime>(qint64{}).value}
        };
        emit refreshPort(m_portConfig["portName"].toString(), session);
        emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "opened");
        return true;
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("open failed: %1").arg(m_serialPort->errorString()));
    return false;
}

void SerialPort::close() {
    // status check
    if (m_serialPort == nullptr) return;
    // port close
    m_serialPort->close();
    if (m_monitorTimer) m_monitorTimer->stop();
    clear();
    const auto &session = QVariantHash{
        {"active", false}
    };
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void SerialPort::clear() {
    m_buffer.clear();
}

void SerialPort::monitor(const bool enabled) {
    if (m_monitorTimer == nullptr || m_serialPort == nullptr || !m_serialPort->isOpen()) return;
    if (enabled) {
        handleUpdate();
        m_monitorTimer->start();
    } else {
        m_monitorTimer->stop();
    }
}

bool SerialPort::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
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

QByteArray SerialPort::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray SerialPort::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
void SerialPort::handleReadyRead() {
    const auto rxData = m_serialPort->readAll();
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
    }
    handleLog(LogLevel::Receive, rxData);
}

void SerialPort::handleError() {
    if (m_serialPort->error() == QSerialPort::NoError || m_serialPort->error() == QSerialPort::TimeoutError) return;
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    if (m_monitorTimer) m_monitorTimer->stop();
    const auto &session = QVariantHash{
        {"active", false}
    };
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("%1").arg(m_serialPort->errorString()));
}

bool SerialPort::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_serialPort == nullptr || !m_serialPort->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    if (m_serialPort->write(f_txData) < 0) return false;
    handleLog(LogLevel::Transmit, f_txData);
    return true;
}

QByteArray SerialPort::handleRead(const int length, const int timeout, const bool peek) {
    // check port status
    if (m_serialPort == nullptr || !m_serialPort->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < length) {
        if (deadline.hasExpired()) break;
        m_serialPort->waitForReadyRead(10);
    }
    return m_buffer.read(length, peek);
}

QByteArray SerialPort::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    // check port status
    if (m_serialPort == nullptr || !m_serialPort->isOpen()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (deadline.hasExpired()) break;
        m_serialPort->waitForReadyRead(10);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void SerialPort::handleUpdate() {
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

void SerialPort::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1] -&gt;").arg(m_serialPort->portName()), message);
    } else {
        emit appendLog(type, QString("[%1] &lt;-").arg(m_serialPort->portName()), message);
    }
}

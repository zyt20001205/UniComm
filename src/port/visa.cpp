#include "visa.h"

#include <QScopedValueRollback>
#include <QTimer>

#include "globals.h"
#include "port/visa.h"
#include "util/uniCast.h"

// public
Visa::Visa(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

Visa::~Visa() {
    close();
}

int Visa::type() {
    return PortType::Visa;
}

QJsonObject Visa::config() {
    return m_portConfig;
}

QVariantHash Visa::info() {
    return {};
}

bool Visa::open() {
    if (m_monitorTimer == nullptr) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(16);
        m_monitorTimer->setSingleShot(false);
        connect(m_monitorTimer, &QTimer::timeout, this, &Visa::handleUpdate);
    }
    ViStatus status = viOpen(g_rm, m_portConfig["portName"].toString().toUtf8().constData(), VI_NULL, VI_NULL, &m_visa);
    if (status == VI_SUCCESS) {
        status = viSetAttribute(m_visa, VI_ATTR_TMO_VALUE, 5000);
        m_buffer.clear();
        m_buffer.resetStatistics();
        m_activeTimer.start();
        const QVariantHash session{
            {"active", true},
            {"capacity", m_portConfig["bufferSize"].toInt()},
            {"lifetime", uni_cast<QLifetime>(qint64{}).value}
        };
        emit refreshPort(m_portConfig["portName"].toString(), session);
        emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "opened");
        // logging
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        qDebug() << QString("[%1] %2 opened").arg(timestamp, m_portConfig["portName"].toString());
        return true;
    }
    emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
    return false;
}

void Visa::close() {
    if (m_visa != VI_NULL) {
        viClose(m_visa);
        m_visa = VI_NULL;
        if (m_monitorTimer) m_monitorTimer->stop();
        const QVariantHash session{{"active", false}};
        emit refreshPort(m_portConfig["portName"].toString(), session);
        emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
    }
}

void Visa::clear() {
    m_buffer.clear();
}

void Visa::monitor(const bool enabled) {
    if (m_monitorTimer == nullptr || m_visa == VI_NULL) return;
    if (enabled) {
        handleUpdate();
        m_monitorTimer->start();
    } else {
        m_monitorTimer->stop();
    }
}

bool Visa::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
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

QByteArray Visa::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout);
}

QByteArray Visa::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout);
}

// private
bool Visa::handleWrite(const QByteArray &f_txData) {
    // check port status
    if (m_visa == VI_NULL) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    ViUInt32 retCount;
    const ViStatus status = viWrite(m_visa, (ViBuf) f_txData.constData(), f_txData.size(), &retCount);
    handleLog(LogLevel::Transmit, f_txData);
    if (status == VI_SUCCESS) {
        return true;
    }
    return false;
}

QByteArray Visa::handleRead(const int length, const int timeout) {
    // // check port status
    // if (m_Visa == nullptr || !m_Visa->isOpen()) {
    //     emit appendLog(QString("%1 is not opened").arg(m_portConfig["portName"].toString()), "error");
    //     return {};
    // }
    // QElapsedTimer timer;
    // timer.start();
    // while (timer.elapsed() <= timeout) {
    //     if (m_Visa->bytesAvailable() == length) {
    //         QByteArray rxData = m_Visa->readAll();
    //         return rxData;
    //     }
    //     m_Visa->waitForReadyRead(10);
    // }
    // emit appendLog(QString("%1 timeout").arg(m_portConfig["portName"].toString()), "error");
    return {};
}

QByteArray Visa::handleReadUntil(const QByteArray &text, const int timeout) {
    return {};
}

void Visa::handleUpdate() {
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

void Visa::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    if (type == LogLevel::Transmit) {
        emit appendLog(type, QString("[%1] -&gt;").arg(m_portConfig["portName"].toString()), message);
    } else {
        emit appendLog(type, QString("[%1] &lt;-").arg(m_portConfig["portName"].toString()), message);
    }
}

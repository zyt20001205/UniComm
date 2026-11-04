#include "port/bluetoothLe.h"

#include <QDeadlineTimer>
#include <QEventLoop>
#include <QScopedValueRollback>
#include <QTimer>

#include <stdexcept>

#include "globals.h"
#include "util/uniCast.h"

namespace {
void waitForData(BluetoothLe *port) {
    QEventLoop eventLoop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(port, &BluetoothLe::readyRead, &eventLoop, &QEventLoop::quit);
    QObject::connect(port, &BluetoothLe::disconnected, &eventLoop, &QEventLoop::quit);
    timer.start(10);
    eventLoop.exec();
}
}

// public
BluetoothLe::BluetoothLe(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig),
      m_buffer(portConfig["bufferSize"].toInt()) {
}

BluetoothLe::~BluetoothLe() {
    close();
}

int BluetoothLe::type() {
    return PortType::BluetoothLe;
}

QJsonObject BluetoothLe::config() {
    return m_portConfig;
}

QVariantHash BluetoothLe::info() {
    return {
        {"status", connectedGet() ? "connected" : "disconnected"},
        {"adapter", m_portConfig["adapterAddress"].toString()},
        {"peripheral", m_portConfig["peripheralAddress"].toString()},
        {"service", m_portConfig["serviceUuid"].toString()},
        {"txCharacteristic", m_portConfig["txCharacteristicUuid"].toString()},
        {"rxCharacteristic", m_portConfig["rxCharacteristicUuid"].toString()},
        {"mtu", QString::number(m_mtu)},
        {"bufferSize", QString::number(m_portConfig["bufferSize"].toInt())},
        {"bufferUsed", QString::number(m_buffer.used())}
    };
}

bool BluetoothLe::open() {
    if (m_monitorTimer == nullptr) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(16);
        m_monitorTimer->setSingleShot(false);
        connect(m_monitorTimer, &QTimer::timeout, this, &BluetoothLe::handleUpdate);
    }
    if (connectedGet()) return true;

    m_closing = false;
    m_subscribed = false;
    m_mtu = 0;
    m_buffer.clear();
    m_buffer.resetStatistics();
    const auto portName = m_portConfig["portName"].toString();
    const auto adapterAddress = m_portConfig["adapterAddress"].toString();
    const auto peripheralAddress = m_portConfig["peripheralAddress"].toString();
    emit appendLog(LogLevel::Info, QString("[%1]").arg(portName), QString("connecting to %1").arg(peripheralAddress));

    try {
        m_adapter = {};
        m_peripheral = {};
        auto adapters = SimpleBLE::Adapter::get_adapters();
        for (auto &adapter: adapters) {
            if (QString::fromStdString(adapter.address()).compare(adapterAddress, Qt::CaseInsensitive) == 0) {
                m_adapter = adapter;
                break;
            }
        }
        if (!m_adapter.initialized()) throw std::runtime_error("adapter not found");

        m_adapter.scan_for(5000);
        const auto peripheralSelect = [this, &peripheralAddress](std::vector<SimpleBLE::Peripheral> peripherals) {
            for (auto &peripheral: peripherals) {
                if (QString::fromStdString(peripheral.address()).compare(peripheralAddress, Qt::CaseInsensitive) == 0) {
                    m_peripheral = peripheral;
                    return true;
                }
            }
            return false;
        };
        if (!peripheralSelect(m_adapter.scan_get_results())
            && !peripheralSelect(m_adapter.get_paired_peripherals())
            && !peripheralSelect(m_adapter.get_connected_peripherals()))
            throw std::runtime_error("peripheral not found");

        m_peripheralName = QString::fromStdString(m_peripheral.identifier());
        m_peripheral.set_callback_on_disconnected([this] { QMetaObject::invokeMethod(this, [this] { handleDisconnected(); }, Qt::QueuedConnection); });
        m_peripheral.connect();
        m_mtu = m_peripheral.mtu();

        const auto serviceUuid = m_portConfig["serviceUuid"].toString().toStdString();
        const auto rxCharacteristicUuid = m_portConfig["rxCharacteristicUuid"].toString().toStdString();
        const auto callback = [this](SimpleBLE::ByteArray payload) {
            const QByteArray rxData(reinterpret_cast<const char *>(payload.data()), static_cast<qsizetype>(payload.size()));
            QMetaObject::invokeMethod(this, [this, rxData] { handleNotification(rxData); }, Qt::QueuedConnection);
        };
        if (m_portConfig["subscribeType"].toString() == "indicate") m_peripheral.indicate(serviceUuid, rxCharacteristicUuid, callback);
        else m_peripheral.notify(serviceUuid, rxCharacteristicUuid, callback);
        m_subscribed = true;

        m_activeTimer.start();
        const QVariantHash session{
            {"active", true},
            {"capacity", m_portConfig["bufferSize"].toInt()},
            {"lifetime", uni_cast<QLifetime>(qint64{}).value}
        };
        emit refreshPort(portName, session);
        emit connected();
        emit appendLog(LogLevel::Info, QString("[%1]").arg(portName), QString("connected to %1 [%2]").arg(m_peripheralName, peripheralAddress));
        return true;
    } catch (const std::exception &exception) {
        m_closing = true;
        try {
            if (m_peripheral.initialized() && m_peripheral.is_connected()) m_peripheral.disconnect();
        } catch (...) {
        }
        emit appendLog(LogLevel::Error, QString("[%1]").arg(portName), QString("open failed: %1").arg(exception.what()));
        return false;
    }
}

void BluetoothLe::close() {
    if (!m_peripheral.initialized()) return;

    m_closing = true;
    const bool connected = connectedGet();
    try {
        if (connected && m_subscribed) m_peripheral.unsubscribe(m_portConfig["serviceUuid"].toString().toStdString(), m_portConfig["rxCharacteristicUuid"].toString().toStdString());
        if (connected) m_peripheral.disconnect();
    } catch (const std::exception &exception) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), exception.what());
    }
    m_subscribed = false;
    if (m_monitorTimer) m_monitorTimer->stop();
    emit refreshPort(m_portConfig["portName"].toString(), QVariantHash{{"active", false}});
    if (connected) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("disconnected from %1").arg(m_portConfig["peripheralAddress"].toString()));
}

void BluetoothLe::clear() {
    m_buffer.clear();
}

void BluetoothLe::monitor(const bool enabled) {
    if (m_monitorTimer == nullptr || !connectedGet()) return;
    if (enabled) {
        handleUpdate();
        m_monitorTimer->start();
    } else {
        m_monitorTimer->stop();
    }
}

bool BluetoothLe::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    if (!txSuffix.isEmpty()) m_portConfig["txSuffix"] = txSuffix;
    QByteArray f_txData = txData;
    if (m_portConfig["txSuffix"].toString() == "crlf") f_txData += "\r\n";
    else if (m_portConfig["txSuffix"].toString() == "modbus crc") f_txData += uni_cast<ModbusCRC>(f_txData).value;
    else if (m_portConfig["txSuffix"].toString() == "modbus lrc") f_txData += uni_cast<ModbusLRC>(f_txData).value;
    return handleWrite(f_txData);
}

QByteArray BluetoothLe::read(const int length, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleRead(length, timeout, peek);
}

QByteArray BluetoothLe::readUntil(const QByteArray &text, const int timeout, const QString &logFormat, const bool peek) {
    QScopedValueRollback configRollback(m_portConfig);
    if (!logFormat.isEmpty()) m_portConfig["logFormat"] = logFormat;
    return handleReadUntil(text, timeout, peek);
}

// private
bool BluetoothLe::connectedGet() {
    try {
        return m_peripheral.initialized() && m_peripheral.is_connected();
    } catch (...) {
        return false;
    }
}

void BluetoothLe::handleDisconnected() {
    if (m_monitorTimer) m_monitorTimer->stop();
    m_subscribed = false;
    emit refreshPort(m_portConfig["portName"].toString(), QVariantHash{{"active", false}});
    emit disconnected();
    if (!m_closing) emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("disconnected from %1").arg(m_portConfig["peripheralAddress"].toString()));
}

void BluetoothLe::handleNotification(const QByteArray &rxData) {
    if (m_closing) return;
    if (m_buffer.write(rxData) != rxData.size()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "buffer overflow");
        close();
        return;
    }
    emit readyRead();
    handleLog(LogLevel::Receive, rxData);
}

bool BluetoothLe::handleWrite(const QByteArray &f_txData) {
    if (!connectedGet()) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return false;
    }
    try {
        const auto serviceUuid = m_portConfig["serviceUuid"].toString().toStdString();
        const auto txCharacteristicUuid = m_portConfig["txCharacteristicUuid"].toString().toStdString();
        const qsizetype packetSize = qMax<qsizetype>(static_cast<qsizetype>(m_mtu) - 3, 20);
        for (qsizetype offset = 0; offset < f_txData.size(); offset += packetSize) {
            const qsizetype size = qMin(packetSize, f_txData.size() - offset);
            const SimpleBLE::ByteArray packet(f_txData.constData() + offset, static_cast<size_t>(size));
            if (m_portConfig["writeType"].toString() == "command") m_peripheral.write_command(serviceUuid, txCharacteristicUuid, packet);
            else m_peripheral.write_request(serviceUuid, txCharacteristicUuid, packet);
        }
        handleLog(LogLevel::Transmit, f_txData);
        return true;
    } catch (const std::exception &exception) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), QString("write failed: %1").arg(exception.what()));
        return false;
    }
}

QByteArray BluetoothLe::handleRead(const int length, const int timeout, const bool peek) {
    const QDeadlineTimer deadline(timeout);
    while (m_buffer.used() < length) {
        if (deadline.hasExpired() || !connectedGet()) break;
        waitForData(this);
    }
    return m_buffer.read(length, peek);
}

QByteArray BluetoothLe::handleReadUntil(const QByteArray &text, const int timeout, const bool peek) {
    if (text.isEmpty()) return {};
    const QDeadlineTimer deadline(timeout);
    QByteArray data = m_buffer.readUntil(text, peek);
    while (data.isEmpty()) {
        if (deadline.hasExpired() || !connectedGet()) break;
        waitForData(this);
        data = m_buffer.readUntil(text, peek);
    }
    return data;
}

void BluetoothLe::handleUpdate() {
    const auto statistics = m_buffer.statistics();
    const QVariantHash session{
        {"used", statistics.used},
        {"lifetime", uni_cast<QLifetime>(m_activeTimer.elapsed()).value},
        {"readCount", statistics.readCount},
        {"readBytes", statistics.readBytes},
        {"writeCount", statistics.writeCount},
        {"writeBytes", statistics.writeBytes}
    };
    emit refreshPort(m_portConfig["portName"].toString(), session);
}

void BluetoothLe::handleLog(const int type, const QByteArray &data) {
    QString message{};
    const QString logFormat = m_portConfig["logFormat"].toString();
    if (logFormat == "raw") {
        message.reserve(data.size() * 4);
        for (const char c: data) message += QString("\\x%1").arg(static_cast<quint8>(c), 2, 16, QChar('0'));
    } else if (logFormat == "hex") message = data.toHex(' ').toUpper();
    else if (logFormat == "ascii") message = QString::fromLatin1(data);
    else message = QString::fromUtf8(data);

    const auto adapterAddress = m_portConfig["adapterAddress"].toString();
    const auto peripheralAddress = m_portConfig["peripheralAddress"].toString();
    if (type == LogLevel::Transmit) emit appendLog(type, QString("[%1 -&gt; %2]").arg(adapterAddress, peripheralAddress), message);
    else emit appendLog(type, QString("[%1 &lt;- %2]").arg(adapterAddress, peripheralAddress), message);
}

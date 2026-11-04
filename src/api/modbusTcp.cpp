#include "api/modbusTcp.h"

#include <QElapsedTimer>
#include <sol/error.hpp>
#include <sol/state_view.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"

// public
ModbusTcp::ModbusTcp(QObject *parent)
    : QObject(parent) {
}

void ModbusTcp::init(const std::string &portName, const int transactionId, const int unitId, const int timeout) {
    const auto port = g_port->m_portHash.constFind(QString::fromStdString(portName));
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");

    m_portName = portName;
    m_transactionId = transactionId;
    m_unitId = unitId;
    m_timeout = timeout;
    m_port = port.value();

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->open()) return "open failed";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

sol::table ModbusTcp::readCoils(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadCoils, startAddr, quantity);
}

sol::table ModbusTcp::readDiscreteInputs(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadDiscreteInputs, startAddr, quantity);
}

std::string ModbusTcp::readHoldingRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadHoldingRegisters, startAddr, quantity);
}

std::string ModbusTcp::readInputRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadInputRegisters, startAddr, quantity);
}

void ModbusTcp::writeSingleCoil(const int coilAddr, const bool value) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    constexpr int protocolId = 0x00;
    constexpr int txLength = 6;
    QByteArray coilValue{};
    coilValue.append(static_cast<char>(value ? 0xFF : 0x00));
    coilValue.append('\0');
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(FuncCode::WriteSingleCoil);
    txData.append(static_cast<qint8>(coilAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(coilAddr & 0xFF));
    txData += coilValue;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleCoil);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus tcp write single coil response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != coilAddr)
        throw sol::error(m_portName + ": modbus tcp write single coil coil address inconsistent");
    if (result.data.sliced(2) != coilValue)
        throw sol::error(m_portName + ": modbus tcp write single coil coil value inconsistent");
}

void ModbusTcp::writeSingleRegister(const int regAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    constexpr int protocolId = 0x00;
    constexpr int txLength = 6;
    const auto value = QByteArray::fromHex(QByteArray::fromStdString(data));
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(FuncCode::WriteSingleRegister);
    txData.append(static_cast<qint8>(regAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(regAddr & 0xFF));
    txData += value;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleRegister);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus tcp write single register response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != regAddr)
        throw sol::error(m_portName + ": modbus tcp write single register register address inconsistent");
    if (result.data.sliced(2) != value)
        throw sol::error(m_portName + ": modbus tcp write single register register value inconsistent");
}

void ModbusTcp::writeMultipleCoils(const int startAddr, const sol::table &values) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    constexpr int protocolId = 0x00;
    const int quantity = static_cast<int>(values.size());
    const int byteCount = (quantity + 7) / 8;
    const int txLength = byteCount + 7;
    QByteArray coilData(byteCount, '\0');
    for (int i = 0; i < quantity; ++i) {
        if (values.get<bool>(i + 1))
            coilData[i / 8] = static_cast<char>(static_cast<quint8>(coilData.at(i / 8)) | 1U << (i % 8));
    }

    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(FuncCode::WriteMultipleCoils);
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    txData.append(static_cast<qint8>(byteCount));
    txData += coilData;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleCoils);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus tcp write multiple coils response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus tcp write multiple coils start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != quantity)
        throw sol::error(m_portName + ": modbus tcp write multiple coils quantity inconsistent");
}

void ModbusTcp::writeMultipleRegisters(const int startAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    constexpr int protocolId = 0x00;
    const auto value = QByteArray::fromHex(QByteArray::fromStdString(data));
    const int txLength = static_cast<int>(value.size()) + 7;
    const int regCount = static_cast<int>(value.size()) / 2;
    const int byteCount = static_cast<int>(value.size());
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(FuncCode::WriteMultipleRegisters);
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(regCount >> 8 & 0xFF));
    txData.append(static_cast<qint8>(regCount & 0xFF));
    txData.append(static_cast<qint8>(byteCount));
    txData += value;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleRegisters);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus tcp write multiple registers response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus tcp write multiple registers start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != regCount)
        throw sol::error(m_portName + ": modbus tcp write multiple registers register count inconsistent");
}

// private
sol::table ModbusTcp::readBits(const sol::this_state ts, const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    constexpr int protocolId = 0x00;
    constexpr int txLength = 6;
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(static_cast<qint8>(funcCode));
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    const int byteCount = (quantity + 7) / 8;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus tcp read bits byte count inconsistent");

    const QByteArray bitData = result.data.sliced(1);
    sol::state_view lua(ts);
    auto values = lua.create_table(quantity, 0);
    for (int i = 0; i < quantity; ++i)
        values[i + 1] = (static_cast<quint8>(bitData.at(i / 8)) & 1U << (i % 8)) != 0;
    return values;
}

std::string ModbusTcp::readRegisters(const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    constexpr int protocolId = 0x00;
    constexpr int txLength = 6;
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_transactionId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(m_transactionId & 0xFF));
    txData.append(static_cast<qint8>(protocolId >> 8 & 0xFF));
    txData.append(static_cast<qint8>(protocolId & 0xFF));
    txData.append(static_cast<qint8>(txLength >> 8 & 0xFF));
    txData.append(static_cast<qint8>(txLength & 0xFF));
    txData.append(static_cast<qint8>(m_unitId));
    txData.append(static_cast<qint8>(funcCode));
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    const int byteCount = quantity * 2;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "hex", "null")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus tcp read registers byte count inconsistent");

    const QByteArray regData = result.data.sliced(1);
    return {regData.constData(), static_cast<std::string::size_type>(regData.size())};
}

ModbusTcp::Result ModbusTcp::response(const int funcCode) const {
    QElapsedTimer timer{};
    timer.start();
    constexpr int prefixSize = 6;
    const auto header = m_port->read(prefixSize, m_timeout, "hex", true);
    if (header.size() != prefixSize) return {.data = {}, .exception = "read timeout"};

    // Length includes the Unit Identifier and PDU, but not the first six bytes.
    const int length = static_cast<quint8>(header.at(4)) << 8 | static_cast<quint8>(header.at(5));
    if (length < 2 || length > 254) return {.data = {}, .exception = "invalid modbus tcp length"};
    const auto remaining = m_timeout < 0 ? -1 : static_cast<int>(qMax<qint64>(0, m_timeout - timer.elapsed()));
    return parser(funcCode, m_port->read(prefixSize + length, remaining, "hex"));
}

ModbusTcp::Result ModbusTcp::parser(const int funcCode, const QByteArray &rxData) const {
    if (rxData.isEmpty()) return {.data = {}, .exception = "read timeout"};
    if (rxData.size() < 9) return {.data = {}, .exception = "invalid modbus tcp response"};

    const int transactionId = static_cast<quint8>(rxData.at(0)) << 8 | static_cast<quint8>(rxData.at(1));
    if (transactionId != m_transactionId) return {.data = {}, .exception = "modbus tcp transaction id inconsistent"};

    const int protocolId = static_cast<quint8>(rxData.at(2)) << 8 | static_cast<quint8>(rxData.at(3));
    if (protocolId != 0) return {.data = {}, .exception = "modbus tcp protocol id inconsistent"};

    const int length = static_cast<quint8>(rxData.at(4)) << 8 | static_cast<quint8>(rxData.at(5));
    if (length != rxData.size() - 6) return {.data = {}, .exception = "modbus tcp length inconsistent"};
    if (static_cast<quint8>(rxData.at(6)) != m_unitId) return {.data = {}, .exception = "modbus tcp unit id inconsistent"};

    const auto rxFuncCode = static_cast<quint8>(rxData.at(7));
    if (rxFuncCode == (funcCode | FuncCode::ExceptionMask))
        return {.data = {}, .exception = "modbus exception(" + QString::number(static_cast<quint8>(rxData.at(8))) + ")"};
    if (rxFuncCode != funcCode) return {.data = {}, .exception = "modbus tcp function code inconsistent"};

    return {.data = rxData.sliced(8), .exception = {}};
}

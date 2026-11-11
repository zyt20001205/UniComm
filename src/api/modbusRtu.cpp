#include "api/modbusRtu.h"

#include <QDeadlineTimer>
#include <sol/error.hpp>
#include <sol/state_view.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "util/uniCast.h"

// public
ModbusRtu::ModbusRtu(QObject *parent)
    : QObject(parent) {
}

void ModbusRtu::init(const std::string &portName, const int slaveAddr, const int timeout) {
    const auto port = g_port->m_portHash.constFind(QString::fromStdString(portName));
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");

    m_portName = portName;
    m_slaveAddr = slaveAddr;
    m_timeout = timeout;
    m_port = port.value();

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->open()) return "open failed";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

sol::table ModbusRtu::readCoils(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadCoils, startAddr, quantity);
}

sol::table ModbusRtu::readDiscreteInputs(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadDiscreteInputs, startAddr, quantity);
}

std::string ModbusRtu::readHoldingRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadHoldingRegisters, startAddr, quantity);
}

std::string ModbusRtu::readInputRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadInputRegisters, startAddr, quantity);
}

void ModbusRtu::writeSingleCoil(const int coilAddr, const bool value) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    QByteArray coilValue{};
    coilValue.append(static_cast<char>(value ? 0xFF : 0x00));
    coilValue.append('\0');
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(FuncCode::WriteSingleCoil);
    txData.append(static_cast<qint8>(coilAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(coilAddr & 0xFF));
    txData += coilValue;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleCoil);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus rtu write single coil response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != coilAddr)
        throw sol::error(m_portName + ": modbus rtu write single coil coil address inconsistent");
    if (result.data.sliced(2) != coilValue)
        throw sol::error(m_portName + ": modbus rtu write single coil coil value inconsistent");
}

void ModbusRtu::writeSingleRegister(const int regAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const auto value = QByteArray::fromHex(QByteArray::fromStdString(data));
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(FuncCode::WriteSingleRegister);
    txData.append(static_cast<qint8>(regAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(regAddr & 0xFF));
    txData += value;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleRegister);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus rtu write single register response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != regAddr)
        throw sol::error(m_portName + ": modbus rtu write single register register address inconsistent");
    if (result.data.sliced(2) != value)
        throw sol::error(m_portName + ": modbus rtu write single register register value inconsistent");
}

void ModbusRtu::writeMultipleCoils(const int startAddr, const sol::table &values) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const int quantity = static_cast<int>(values.size());
    const int byteCount = (quantity + 7) / 8;
    QByteArray coilData(byteCount, '\0');
    for (int i = 0; i < quantity; ++i) {
        if (values.get<bool>(i + 1))
            coilData[i / 8] = static_cast<char>(static_cast<quint8>(coilData.at(i / 8)) | 1U << (i % 8));
    }

    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(FuncCode::WriteMultipleCoils);
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    txData.append(static_cast<qint8>(byteCount));
    txData += coilData;

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleCoils);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus rtu write multiple coils response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus rtu write multiple coils start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != quantity)
        throw sol::error(m_portName + ": modbus rtu write multiple coils quantity inconsistent");
}

void ModbusRtu::writeMultipleRegisters(const int startAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const auto size = static_cast<qsizetype>(data.size() / 2);
    const int regCount = static_cast<int>(size) / 2;
    const int byteCount = static_cast<int>(size);
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(FuncCode::WriteMultipleRegisters);
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(regCount >> 8 & 0xFF));
    txData.append(static_cast<qint8>(regCount & 0xFF));
    txData.append(static_cast<qint8>(byteCount));
    txData += QByteArray::fromHex(QByteArray::fromStdString(data));

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleRegisters);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus rtu write multiple registers response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus rtu write multiple registers start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != regCount)
        throw sol::error(m_portName + ": modbus rtu write multiple registers register count inconsistent");
}

// private
sol::table ModbusRtu::readBits(const sol::this_state ts, const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(static_cast<qint8>(funcCode));
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    const int byteCount = (quantity + 7) / 8;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus rtu read bits byte count inconsistent");

    const QByteArray bitData = result.data.sliced(1);
    sol::state_view lua(ts);
    auto values = lua.create_table(quantity, 0);
    for (int i = 0; i < quantity; ++i)
        values[i + 1] = (static_cast<quint8>(bitData.at(i / 8)) & 1U << (i % 8)) != 0;
    return values;
}

std::string ModbusRtu::readRegisters(const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    QByteArray txData{};
    txData.append(static_cast<qint8>(m_slaveAddr));
    txData.append(static_cast<qint8>(funcCode));
    txData.append(static_cast<qint8>(startAddr >> 8 & 0xFF));
    txData.append(static_cast<qint8>(startAddr & 0xFF));
    txData.append(static_cast<qint8>(quantity >> 8 & 0xFF));
    txData.append(static_cast<qint8>(quantity & 0xFF));
    const int byteCount = quantity * 2;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "hex", "modbus crc")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus rtu read registers byte count inconsistent");

    const QByteArray regData = result.data.sliced(1);
    return {regData.constData(), static_cast<std::string::size_type>(regData.size())};
}

ModbusRtu::Result ModbusRtu::response(const int funcCode) const {
    const QDeadlineTimer deadline(m_timeout);
    const auto header = m_port->read(2, static_cast<int>(deadline.remainingTime()), "hex", true);
    if (header.isEmpty()) return {.data = {}, .exception = "read timeout"};

    const auto rxFuncCode = static_cast<quint8>(header.at(1));
    int length{};
    if (rxFuncCode & FuncCode::ExceptionMask) length = 5;
    else {
        switch (rxFuncCode) {
            case FuncCode::ReadCoils:
            case FuncCode::ReadDiscreteInputs:
            case FuncCode::ReadHoldingRegisters:
            case FuncCode::ReadInputRegisters: {
                const auto data = m_port->read(3, static_cast<int>(deadline.remainingTime()), "hex", true);
                if (data.isEmpty()) return {.data = {}, .exception = "read timeout"};
                length = static_cast<quint8>(data.at(2)) + 5;
                break;
            }
            case FuncCode::WriteSingleCoil:
            case FuncCode::WriteSingleRegister:
            case FuncCode::WriteMultipleCoils:
            case FuncCode::WriteMultipleRegisters:
                length = 8;
                break;
            default:
                return {.data = {}, .exception = "unsupported modbus rtu response function code"};
        }
    }

    if (length > 256) return {.data = {}, .exception = "invalid modbus rtu length"};
    return parser(funcCode, m_port->read(length, static_cast<int>(deadline.remainingTime()), "hex", false));
}

ModbusRtu::Result ModbusRtu::parser(const int funcCode, const QByteArray &rxData) const {
    if (rxData.isEmpty()) return {.data = {}, .exception = "read timeout"};
    if (rxData.size() < 5) return {.data = {}, .exception = "invalid modbus rtu response"};
    if (rxData.right(2) != uni_cast<ModbusCRC>(rxData.chopped(2)).value) return {.data = {}, .exception = "modbus rtu checksum error"};
    if (static_cast<quint8>(rxData.at(0)) != m_slaveAddr) return {.data = {}, .exception = "modbus rtu slave address inconsistent"};

    const auto rxFuncCode = static_cast<quint8>(rxData.at(1));
    if (rxFuncCode == (funcCode | FuncCode::ExceptionMask))
        return {.data = {}, .exception = "modbus exception(" + QString::number(static_cast<quint8>(rxData.at(2))) + ")"};
    if (rxFuncCode != funcCode) return {.data = {}, .exception = "modbus rtu function code inconsistent"};

    return {.data = rxData.sliced(2, rxData.size() - 4), .exception = {}};
}

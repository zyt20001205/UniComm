#include "api/modbusAscii.h"

#include <sol/error.hpp>
#include <sol/state_view.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "util/uniCast.h"

// public
ModbusAscii::ModbusAscii(QObject *parent)
    : QObject(parent) {
}

void ModbusAscii::init(const std::string &portName, const int slaveAddr, const int timeout) {
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

sol::table ModbusAscii::readCoils(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadCoils, startAddr, quantity);
}

sol::table ModbusAscii::readDiscreteInputs(const sol::this_state ts, const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readBits(ts, FuncCode::ReadDiscreteInputs, startAddr, quantity);
}

std::string ModbusAscii::readHoldingRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadHoldingRegisters, startAddr, quantity);
}

std::string ModbusAscii::readInputRegisters(const int startAddr, const int quantity) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    return readRegisters(FuncCode::ReadInputRegisters, startAddr, quantity);
}

void ModbusAscii::writeSingleCoil(const int coilAddr, const bool value) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const QByteArray coilValue = value ? QByteArray::fromHex("FF00") : QByteArray::fromHex("0000");
    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(FuncCode::WriteSingleCoil, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(coilAddr, 16).rightJustified(4, '0').toUpper();
    txData += coilValue.toHex().toUpper();

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleCoil);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus ascii write single coil response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != coilAddr)
        throw sol::error(m_portName + ": modbus ascii write single coil coil address inconsistent");
    if (result.data.sliced(2) != coilValue)
        throw sol::error(m_portName + ": modbus ascii write single coil coil value inconsistent");
}

void ModbusAscii::writeSingleRegister(const int regAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const auto value = QByteArray::fromHex(QByteArray::fromStdString(data));
    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(FuncCode::WriteSingleRegister, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(regAddr, 16).rightJustified(4, '0').toUpper();
    txData += value.toHex().toUpper();

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteSingleRegister);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus ascii write single register response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != regAddr)
        throw sol::error(m_portName + ": modbus ascii write single register register address inconsistent");
    if (result.data.sliced(2) != value)
        throw sol::error(m_portName + ": modbus ascii write single register register value inconsistent");
}

void ModbusAscii::writeMultipleCoils(const int startAddr, const sol::table &values) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const int quantity = static_cast<int>(values.size());
    const int byteCount = (quantity + 7) / 8;
    QByteArray coilData(byteCount, '\0');
    for (int i = 0; i < quantity; ++i) {
        if (values.get<bool>(i + 1))
            coilData[i / 8] = static_cast<char>(static_cast<quint8>(coilData.at(i / 8)) | 1U << (i % 8));
    }

    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(FuncCode::WriteMultipleCoils, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(startAddr, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(quantity, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(byteCount, 16).rightJustified(2, '0').toUpper();
    txData += coilData.toHex().toUpper();

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleCoils);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus ascii write multiple coils response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus ascii write multiple coils start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != quantity)
        throw sol::error(m_portName + ": modbus ascii write multiple coils quantity inconsistent");
}

void ModbusAscii::writeMultipleRegisters(const int startAddr, const std::string &data) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    Result result{};
    const auto value = QByteArray::fromHex(QByteArray::fromStdString(data));
    const int regCount = static_cast<int>(value.size()) / 2;
    const int byteCount = static_cast<int>(value.size());
    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(FuncCode::WriteMultipleRegisters, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(startAddr, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(regCount, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(byteCount, 16).rightJustified(2, '0').toUpper();
    txData += value.toHex().toUpper();

    QMetaObject::invokeMethod(m_port, [this, &txData] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(FuncCode::WriteMultipleRegisters);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != 4)
        throw sol::error(m_portName + ": invalid modbus ascii write multiple registers response");
    if ((static_cast<quint8>(result.data.at(0)) << 8 | static_cast<quint8>(result.data.at(1))) != startAddr)
        throw sol::error(m_portName + ": modbus ascii write multiple registers start address inconsistent");
    if ((static_cast<quint8>(result.data.at(2)) << 8 | static_cast<quint8>(result.data.at(3))) != regCount)
        throw sol::error(m_portName + ": modbus ascii write multiple registers register count inconsistent");
}

// private
sol::table ModbusAscii::readBits(const sol::this_state ts, const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(funcCode, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(startAddr, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(quantity, 16).rightJustified(4, '0').toUpper();
    const int byteCount = (quantity + 7) / 8;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus ascii read bits byte count inconsistent");

    const QByteArray bitData = result.data.sliced(1);
    sol::state_view lua(ts);
    auto values = lua.create_table(quantity, 0);
    for (int i = 0; i < quantity; ++i)
        values[i + 1] = (static_cast<quint8>(bitData.at(i / 8)) & 1U << (i % 8)) != 0;
    return values;
}

std::string ModbusAscii::readRegisters(const int funcCode, const int startAddr, const int quantity) const {
    Result result{};
    QByteArray txData = ":";
    txData += QByteArray::number(m_slaveAddr, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(funcCode, 16).rightJustified(2, '0').toUpper();
    txData += QByteArray::number(startAddr, 16).rightJustified(4, '0').toUpper();
    txData += QByteArray::number(quantity, 16).rightJustified(4, '0').toUpper();
    const int byteCount = quantity * 2;

    QMetaObject::invokeMethod(m_port, [this, &txData, funcCode] -> Result {
        if (!m_port->write(txData, "ascii", "modbus lrc")) return {.data = {}, .exception = "write failed"};

        return response(funcCode);
    }, Qt::BlockingQueuedConnection, &result);
    if (!result.exception.isEmpty()) throw sol::error(m_portName + ": " + result.exception.toStdString());

    if (result.data.size() != byteCount + 1 || static_cast<quint8>(result.data.at(0)) != byteCount)
        throw sol::error(m_portName + ": modbus ascii read registers byte count inconsistent");

    const QByteArray regData = result.data.sliced(1);
    return {regData.constData(), static_cast<std::string::size_type>(regData.size())};
}

ModbusAscii::Result ModbusAscii::response(const int funcCode) const {
    return parser(funcCode, m_port->readUntil("\r\n", m_timeout, "ascii"));
}

ModbusAscii::Result ModbusAscii::parser(const int funcCode, const QByteArray &rxData) const {
    if (rxData.isEmpty()) return {.data = {}, .exception = "read timeout"};
    if (rxData.size() < 11) return {.data = {}, .exception = "invalid modbus ascii response"};
    if (rxData.at(0) != ':') return {.data = {}, .exception = "modbus ascii colon missing"};
    if (rxData.right(4) != uni_cast<ModbusLRC>(rxData.chopped(4)).value) return {.data = {}, .exception = "modbus ascii checksum error"};
    if (rxData.sliced(1, 2).toUInt(nullptr, 16) != m_slaveAddr) return {.data = {}, .exception = "modbus ascii slave address inconsistent"};

    const auto rxFuncCode = rxData.sliced(3, 2).toUInt(nullptr, 16);
    if (rxFuncCode == (funcCode | FuncCode::ExceptionMask))
        return {.data = {}, .exception = "modbus exception(" + QString::number(rxData.sliced(5, 2).toUInt(nullptr, 16)) + ")"};
    if (rxFuncCode != funcCode) return {.data = {}, .exception = "modbus ascii function code inconsistent"};

    return {.data = QByteArray::fromHex(rxData.sliced(5, rxData.size() - 9)), .exception = {}};
}

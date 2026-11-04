#ifndef UNICOMM_MODBUSASCII_H
#define UNICOMM_MODBUSASCII_H

#include <QObject>
#include <QPointer>
#include <sol/table.hpp>

class BasePort;

class ModbusAscii final : public QObject {
    Q_OBJECT

public:
    explicit ModbusAscii(QObject *parent = nullptr);

    ~ModbusAscii() override = default;

    void init(const std::string &portName, int slaveAddr, int timeout);

    [[nodiscard]] sol::table readCoils(sol::this_state ts, int startAddr, int quantity) const;

    [[nodiscard]] sol::table readDiscreteInputs(sol::this_state ts, int startAddr, int quantity) const;

    [[nodiscard]] std::string readHoldingRegisters(int startAddr, int quantity) const;

    [[nodiscard]] std::string readInputRegisters(int startAddr, int quantity) const;

    void writeSingleCoil(int coilAddr, bool value) const;

    void writeSingleRegister(int regAddr, const std::string &data) const;

    void writeMultipleCoils(int startAddr, const sol::table &values) const;

    void writeMultipleRegisters(int startAddr, const std::string &data) const;

private:
    struct FuncCode {
        // https://www.modbus.org/file/secure/modbusprotocolspecification.pdf
        enum {
            ReadCoils = 0x01,
            ReadDiscreteInputs = 0x02,
            ReadHoldingRegisters = 0x03,
            ReadInputRegisters = 0x04,
            WriteSingleCoil = 0x05,
            WriteSingleRegister = 0x06,
            ReadExceptionStatus = 0x07,
            Diagnostics = 0x08,

            GetCommEventCounter = 0x0B,
            GetCommEventLog = 0x0C,

            WriteMultipleCoils = 0x0F,
            WriteMultipleRegisters = 0x10,
            ReportServerID = 0x11,

            ReadFileRecord = 0x14,
            WriteFileRecord = 0x15,
            MaskWriteRegister = 0x16,
            ReadWriteMultipleRegisters = 0x17,
            ReadFifoQueue = 0x18,

            EncapsulatedInterfaceTransport = 0x2B
        };

        static constexpr int ExceptionMask = 0x80;
    };

    struct Result {
        QByteArray data{};
        QString exception{};
    };

    [[nodiscard]] sol::table readBits(sol::this_state ts, int funcCode, int startAddr, int quantity) const;

    [[nodiscard]] std::string readRegisters(int funcCode, int startAddr, int quantity) const;

    [[nodiscard]] Result response(int funcCode) const;

    [[nodiscard]] Result parser(int funcCode, const QByteArray &rxData) const;

    std::string m_portName{};
    int m_slaveAddr{};
    int m_timeout{};
    QPointer<BasePort> m_port{};
};

#endif //UNICOMM_MODBUSASCII_H

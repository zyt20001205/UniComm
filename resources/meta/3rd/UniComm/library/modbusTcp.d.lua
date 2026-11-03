---@meta

---@class ModbusTcp
ModbusTcp = {}

---
---Create a Modbus TCP instance and open the configured port.
---
---[Modbus TCP demo](../demo/modbusTcp.lua)
---
---@param name portName
---@param transactionId integer The transaction id used for pairing.
---@param unitId integer The unit id (1-247) of the target device on the network.
---@param timeout? integer (default: 1000) Maximum time in **milliseconds** to wait for data to arrive.
---@return modbusTcp
function ModbusTcp.new(name, transactionId, unitId, timeout) end

---@class modbusTcp
modbusTcp = {}

---
---Read the states of multiple coils from a Modbus TCP device.
---
---@param startAddr integer The starting address of the first coil to read.
---@param quantity integer Number of coils to read.
---@return boolean[] values The coil states. The first value corresponds to `startAddr`.
function modbusTcp:readCoils(startAddr, quantity) end

---
---Read the states of multiple discrete inputs from a Modbus TCP device.
---
---@param startAddr integer The starting address of the first discrete input to read.
---@param quantity integer Number of discrete inputs to read.
---@return boolean[] values The input states. The first value corresponds to `startAddr`.
function modbusTcp:readDiscreteInputs(startAddr, quantity) end

---
---Read data from multiple holding registers of a Modbus TCP device.
---
---@param startAddr integer The starting address of the first register to read from.
---@param quantity integer Number of registers to read.
---@return string
function modbusTcp:readHoldingRegisters(startAddr, quantity) end

---
---Read data from multiple input registers of a Modbus TCP device.
---
---@param startAddr integer The starting address of the first register to read from.
---@param quantity integer Number of registers to read.
---@return string
function modbusTcp:readInputRegisters(startAddr, quantity) end

---
---Write a state to a single coil of a Modbus TCP device.
---
---@param coilAddr integer The address of the coil to write to.
---@param value boolean The coil state to write.
---@return nil
function modbusTcp:writeSingleCoil(coilAddr, value) end

---
---Write data to a single register of a Modbus TCP device.
---
---@param regAddr integer The address of the register to write to.
---@param data string **Hex string** containing the data to be written.
---@return nil
function modbusTcp:writeSingleRegister(regAddr, data) end

---
---Write states to multiple coils of a Modbus TCP device.
---
---@param startAddr integer The starting address of the first coil to write to.
---@param values boolean[] The coil states to write. The first value corresponds to `startAddr`.
---@return nil
function modbusTcp:writeMultipleCoils(startAddr, values) end

---
---Write data to multiple holding registers of a Modbus TCP device.
---
---@param startAddr integer The starting address of the first register to write to.
---@param data string **Hex string** containing the data to be written.
---@return nil
function modbusTcp:writeMultipleRegisters(startAddr, data) end

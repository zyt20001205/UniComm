-- Configure a TCP client port named "Modbus TCP" before running.
-- new() opens the configured port automatically.
local modbusTcp = ModbusTcp.new("Modbus TCP", 1, 1, 1000)

-- Read multiple coil states.
local coils = modbusTcp:readCoils(0, 10)

-- Read multiple discrete input states.
local inputs = modbusTcp:readDiscreteInputs(0, 10)

-- Read a single 16-bit register.
local raw = modbusTcp:readHoldingRegisters(0, 1)
local data = string.unpack(">i2", raw)

-- Read two consecutive 16-bit registers and combine them into a 32-bit value.
local raw = modbusTcp:readHoldingRegisters(0, 2)
local data = string.unpack(">i4", raw)

-- Read two consecutive 16-bit input registers.
local raw = modbusTcp:readInputRegisters(0, 2)
local data = string.unpack(">i4", raw)

-- Write a single coil state.
modbusTcp:writeSingleCoil(0, true)

-- Write a single 16-bit value.
local raw = string.pack(">i2", 200)
modbusTcp:writeSingleRegister(0, string.toHex(raw))

-- Write multiple coil states.
modbusTcp:writeMultipleCoils(0, {
    true,
    false,
    true,
    true
})

-- Write multiple consecutive 16-bit registers.
local raw = string.pack(">i4", 1000000)
modbusTcp:writeMultipleRegisters(0, string.toHex(raw))

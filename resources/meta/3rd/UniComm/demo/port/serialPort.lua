-- Replace COM4 with your device name. The logical portName can be chosen freely.
local serial = Port.create({
    portType = Port.Type.SerialPort,
    portName = "Serial Port",
    deviceName = "COM4",
    baudRate = 115200,
    logFormat = "utf-8",
})

serial:open()
print(serial:info())

-- Replace the line with a request defined by your device's protocol.
serial:write("Hello from UniComm\r\n")
print(serial:readUntil("\r\n", 1000))
serial:close()

-- To reuse this saved configuration on a later run:
-- local serial = Port.get("Serial Port")

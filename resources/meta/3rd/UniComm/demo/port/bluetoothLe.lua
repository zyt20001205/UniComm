-- Discover and save a port named "BLE Device" in port settings before running.
local ble = Port.get("BLE Device")

-- Alternatively, create a port using the addresses from discovery:
-- local ble = Port.create({
--     portType = Port.Type.BluetoothLe,
--     portName = "BLE Device",
--     adapterAddress = "00:11:22:33:44:55",
--     peripheralAddress = "AA:BB:CC:DD:EE:FF",
--     logFormat = "hex",
-- })

ble:open()
for service, characteristics in pairs(ble:info().services) do
    for characteristic, details in pairs(characteristics) do
        print(service, characteristic)
        print("Capabilities:", details.capabilities)
        print("Descriptors:", details.descriptors)
    end
end

-- Read GAP Device Name directly, when the device exposes this readable characteristic.
-- Short UUIDs are expanded automatically. This does not use a subscription buffer.
local name = ble:read("1800", "2a00")
print("Device name:", name)

-- Optional descriptor read: use a descriptor listed under the chosen characteristic.
-- local description = ble:read("1800", "2a00", "2901")
-- print(description)

-- The following operations need UUIDs and a command defined by your device.
-- Uncomment only after replacing these example UUIDs and confirming capabilities.
--[[
local service = "12345678-1234-5678-1234-56789abcdef0"
local characteristic = "12345678-1234-5678-1234-56789abcdef1"
local command = string.fromHex("01")

-- 1 = Write Request; use 2 for Write Command if the characteristic supports it.
ble:write(command, 1, service, characteristic)

-- 3 = Notify; use 4 for Indicate if supported. Updates fill this characteristic's buffer.
ble:subscribe(3, service, characteristic)
local data = ble:read(0, 2000, service, characteristic)
print(string.toHex(data, " "))
ble:unsubscribe(service, characteristic)

-- Descriptor writes have no mode. Only write documented writable descriptors.
-- ble:write("Data channel", service, characteristic, "2901")
]]

-- Use subscribe/unsubscribe, not descriptor writes to CCCD (2902), for subscriptions.
ble:close()

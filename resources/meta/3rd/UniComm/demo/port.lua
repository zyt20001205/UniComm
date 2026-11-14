-- A port can be configured in the UI or created by Lua. The supported
-- configurations are shown below. Copy one block, replace its connection fields,
-- then operate on the returned port instance.

-- Serial port:
-- local serial = Port.create({
--     portType = Port.Type.SerialPort,
--     portName = "Serial Port",
--     deviceName = "COM4",
--     baudRate = 115200,
--     logFormat = "utf-8",
-- })
-- serial:open()

-- TCP client:
-- local tcp = Port.create({
--     portType = Port.Type.TcpClient,
--     portName = "TcpEcho",
--     remoteHost = "127.0.0.1",
--     remotePort = 8000,
--     logFormat = "utf-8",
-- })
-- tcp:open()

-- TCP server:
-- local tcpServer = Port.create({
--     portType = Port.Type.TcpServer,
--     portName = "TcpServer",
--     localHost = "0.0.0.0",
--     localPort = 8000,
--     logFormat = "utf-8",
-- })
-- tcpServer:open()

-- SSL client:
-- local ssl = Port.create({
--     portType = Port.Type.SslClient,
--     portName = "Bark",
--     remoteHost = "api.day.app",
--     remotePort = 443,
--     logFormat = "utf-8",
-- })
-- ssl:open()

-- SSL server:
-- local sslServer = Port.create({
--     portType = Port.Type.SslServer,
--     portName = "SslServer",
--     localHost = "0.0.0.0",
--     localPort = 8443,
--     certificate = "server-cert.pem",
--     privateKey = "server-key.pem",
--     logFormat = "utf-8",
-- })
-- sslServer:open()

-- WebSocket client: use a complete ws:// or wss:// URL.
-- Type is selected on each write: 1 = UTF-8 text, 2 = binary (default).
-- See websocket.lua for a complete local client/server test without public echo.
-- local ws = Port.create({
--     portType = Port.Type.WebSocketClient,
--     portName = "WebSocket Client",
--     url = "ws://127.0.0.1:8080/",
--     logFormat = "utf-8",
-- })
-- ws:open()
-- ws:write("Hello from UniComm", 1)
-- ws:write(string.fromHex("00 FF 80"), 2)
-- ws:write("Binary by default")
-- ws:close()

-- WebSocket server: accepts ws:// connections.
-- local wsServer = Port.create({
--     portType = Port.Type.WebSocketServer,
--     portName = "WebSocket Server",
--     localHost = "127.0.0.1",
--     localPort = 8080,
--     logFormat = "utf-8",
-- })
-- wsServer:open()
-- After clients connect, omitting peerIp broadcasts to every connected peer.
-- wsServer:write("Hello from UniComm", 1)
-- wsServer:write(string.fromHex("00 FF 80"), 2)

-- UDP socket: localPort = 0 lets the system choose a local port.
-- local udp = Port.create({
--     portType = Port.Type.UdpSocket,
--     portName = "UdpEcho",
--     localHost = "127.0.0.1",
--     localPort = 0,
--     remoteHost = "127.0.0.1",
--     remotePort = 9000,
--     logFormat = "utf-8",
-- })
-- udp:open()
-- print(udp:info().localPort) -- Actual local port selected by the system.

-- Bluetooth LE: copy both addresses from a BLE port configuration saved through port settings.
-- Service/Characteristic UUIDs are selected per operation, not fixed in the configuration.
-- Write modes: 1 = Request, 2 = Command. Subscription modes: 3 = Notify, 4 = Indicate.
-- local ble = Port.create({
--     portType = Port.Type.BluetoothLe,
--     portName = "BLE Device",
--     adapterAddress = "00:11:22:33:44:55",
--     peripheralAddress = "C4:DE:E2:B1:6F:62",
-- })
-- ble:open() -- Scans only if needed, stopping on a match; then waits for the GATT connection.
-- local information = ble:info()
-- print(information)
-- for service, characteristics in pairs(information.services) do
--     for characteristic, details in pairs(characteristics) do
--         print(service, characteristic)
--         print(details.capabilities) -- Capability names for this characteristic.
--         print(details.descriptors) -- Descriptor UUIDs only; no values are read.
--     end
-- end
-- Read the standard GAP Device Name characteristic when it supports reading.
-- local serviceUuid = "1800"
-- local characteristicUuid = "2a00"
-- local value = ble:read(serviceUuid, characteristicUuid)
-- print(value)
-- print(string.toHex(value, " "))
-- On the example Automation IO device, Number of Digitals reports one channel as the byte 01.
-- Confirm these UUIDs appear in info() before running this example.
-- local digitals = ble:read("1815", "2a56", "2909")
-- print(string.toHex(digitals, " "))
-- assert(#digitals == 1 and digitals:byte(1) == 1, "expected one digital channel")
-- A Descriptor read does not require its characteristic to support Read.
-- For example, read the CCCD of an Indicate-only Service Changed characteristic if present:
-- local cccd = ble:read("1801", "2a05", "2902")
-- print(string.toHex(cccd, " "))
-- Descriptor writes have no mode. Only write a value documented for a writable Descriptor.
-- local descriptorData = nil -- Set only if this characteristic has a writable User Description (2901).
-- if descriptorData then
--     ble:write(descriptorData, serviceUuid, characteristicUuid, "2901")
-- end
-- Do not write Number of Digitals (2909): it is read-only. Use subscribe/unsubscribe, not writes to CCCD (2902).
-- Only write commands defined by the device protocol. Do not echo arbitrary read data back.
-- local command = nil -- Replace only with a documented binary command; nil sends nothing.
-- if command then
--     ble:write(command, 1, "00ff", "ff01") -- Write Request: waits for GATT acknowledgement.
--     -- Alternative: ble:write(command, 2, "00ff", "ff01") -- Write Command: no GATT response.
-- end
-- Subscribe only to a characteristic listed by info() with the selected capability.
-- Replace these example UUIDs with the device's documented data channel.
-- local service = "00ff"
-- local characteristic = "ff01"
-- ble:subscribe(3, service, characteristic) -- Notify; use 4 for a characteristic supporting Indicate.
-- ble:subscribe(3, service, characteristic) -- Same mode: keeps the existing buffer and its data.
-- print(ble:info().subscriptions)
-- Updates are appended asynchronously, even when Lua is not currently reading.
-- local preview = ble:peek(0, 2000, service, characteristic)
-- print(string.toHex(preview, " "))
-- local received = ble:read(#preview, 0, service, characteristic)
-- print(string.toHex(received, " "))
-- Choose lengths and delimiters from the device protocol, not the notification size or MTU.
-- local frame = ble:read(20, 2000, service, characteristic) -- Wait for exactly 20 bytes.
-- local line = ble:peekUntil("\r\n", 2000, service, characteristic)
-- local consumed = ble:readUntil("\r\n", 0, service, characteristic)
-- ble:clear() -- Clears every subscription buffer without unsubscribing.
-- ble:unsubscribe(service, characteristic) -- Discards this characteristic's buffer only.
-- ble:unsubscribe(service, characteristic) -- Already unsubscribed: no operation while connected.
-- assert(ble:read(0, 0, service, characteristic) == "")
-- ble:open() -- Already connected: returns without reconnecting.
-- ble:close()
-- ble:close() -- Already closed: no operation.
-- Port.remove("BLE Device") -- Optional: also remove the saved port configuration.

-- Vision ports configured in the UI can also export and read processed ROIs.
-- ROI indexes are 1-based and relative paths resolve from the current workspace.
-- local vision = Port.get("CG345UK")
-- vision:open()
-- vision:write([[roi 1 "capture/roi-1.png"]])
-- vision:write([[processed 1 "capture/processed-1.png"]])
-- vision:write([[bundle "capture/bundle"]])
-- local results = vision:read(0, 20000)
-- print(results)
-- vision:close()

-- Configure an echo-capable single-buffer byte port named "Echo" before running:
-- serial, UDP, TCP/SSL client, or WebSocket client (not a server port).
local name = "Echo"
local timeout = 1000
local port = Port.get(name)

-- Discover configured ports and inspect the selected port.
print(Port.list())
print(port:info())

port:open()
port:clear()

-- peekUntil includes the delimiter but leaves the complete line buffered.
local line = "Hello from UniComm\r\n"
port:write(line)
local preview = port:peekUntil("\r\n", timeout)
assert(preview == line, "echo line was not received")
assert(port:peekUntil() == preview, "peekUntil must not consume the line")

-- readUntil consumes that same line, including its delimiter.
local receivedLine = port:readUntil("\r\n", timeout)
assert(receivedLine == preview, "peekUntil changed the buffered line")
print(receivedLine)

-- peek and read both wait for exactly the requested number of bytes.
-- Only read consumes them. This request also contains null bytes.
local request = string.fromHex("01 03 00 00 00 01")
port:write(request)
assert(port:peek(#request, timeout) == request, "echo bytes were not received")
assert(port:peek() == request, "peek must not consume the bytes")
local response = port:read(#request, timeout)
---@cast response string
assert(response == request, "peek changed the buffered bytes")
print(string.toHex(response, " "))

-- Incomplete data stays buffered after an unsuccessful nonblocking peek.
local partial = "partial"
port:write(partial)
assert(port:peek(#partial, timeout) == partial, "partial echo was not received")
assert(port:peek(#partial + 1) == "", "an incomplete fixed-length peek must return empty")
assert(port:peekUntil("\r\n") == "", "a missing delimiter must return empty")
assert(port:read(#partial, timeout) == partial, "unsuccessful peeks must preserve data")

-- port:peek() immediately copies all currently buffered bytes without consuming.
-- port:read() immediately drains all bytes currently in the receive buffer.
-- With a timeout, both wait for any bytes only when the receive buffer is empty.
-- local preview = port:peek(0, timeout) -- Copies all available bytes once any arrive.
-- local data = port:read(0, timeout)    -- Consumes all available bytes once any arrive.

-- TCP, SSL, and WebSocket servers identify a peer as "address:port".
-- Build the identifier from peerAddress and peerPort returned by port:info().
-- Omitting peerIp from port:write broadcasts; omitting it from read/peek selects
-- one unspecified connected peer. Keep peerIp explicit when pairing peek/read.
-- local server = Port.get("Server")
-- local information = server:info()
-- local peer = information.peer1
-- local peerIp = peer.peerAddress .. ":" .. peer.peerPort
-- server:write("Hello", peerIp)       -- TCP/SSL server.
-- server:write("Hello", 1, peerIp)    -- WebSocket server: text to one peer.
-- server:write("Hello", 1)            -- WebSocket server: text broadcast.
-- server:write(string.fromHex("00 FF"), 2, peerIp) -- WebSocket binary to one peer.
-- local preview = server:peek(5, timeout, peerIp)
-- local data = server:read(5, timeout, peerIp)

-- Already buffered data survives close on the five single-buffer port types above.
-- A repeated open while active must not discard data either.
local tailLine = "buffered before close\r\n"
local tail = tailLine .. partial
port:write(tail)
assert(port:peek(#tail, timeout) == tail, "tail echo was not received")
port:open()
assert(port:peek() == tail, "repeated open must preserve buffered data")
port:close()

-- Complete cached requests still succeed after close, even with timeout = -1.
assert(port:peekUntil("\r\n", -1) == tailLine, "closed port must allow peeking at a cached line")
assert(port:readUntil("\r\n", -1) == tailLine, "closed port must allow reading a cached line")

-- Unsatisfied requests return empty immediately and preserve the remaining tail.
assert(port:peek(#partial + 1, -1) == "", "closed port must not wait for more bytes")
assert(port:read(#partial + 1, -1) == "", "closed port must not return a partial fixed-length read")
assert(port:peekUntil("\r\n", -1) == "", "closed port must not wait for a missing delimiter")
assert(port:readUntil("\r\n", -1) == "", "closed port must preserve an incomplete line")
assert(port:peek() == partial, "unsatisfied requests must preserve the tail")
assert(port:read(0) == partial, "read(0) must drain the closed port's tail")
assert(port:read(1, -1) == "", "an empty closed port must return immediately")
assert(port:peek(1, -1) == "", "an empty closed port must not wait when peeking")

-- A fresh open starts a new session and clears any old unread data.
-- Explicit clear() still discards buffered data without reopening the port.

-- Remove a port created by Lua when it is no longer needed.
-- Port.remove(name)

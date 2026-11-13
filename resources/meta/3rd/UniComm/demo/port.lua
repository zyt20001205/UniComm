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
-- serial, UDP, BLE, TCP/SSL client, or WebSocket client (not a server port).
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

-- Already buffered data survives close on the six single-buffer port types above.
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

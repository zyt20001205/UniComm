-- A port can be configured in the UI or created by Lua. The supported
-- configurations are shown below. Copy one block, replace its connection fields,
-- then operate on the returned port instance.

-- Serial port:
-- local serial = Port.create({
--     portType = Port.Type.SerialPort,
--     portName = "COM4",
--     baudRate = 115200,
--     logFormat = "utf-8",
-- })
-- serial:open()

-- Vision input from an available screen or camera. When no ROI is configured,
-- the complete source frame is processed as ROI 1.
-- local vision = Port.create({
--     portType = Port.Type.Vision,
--     portName = "CG345UK",
-- })
-- vision:open()
-- vision:write([[raw "capture/raw.png"]])
-- local results = vision:read(0, 20000)
-- print(results[1])
-- vision:close()

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

-- Configure an echo-capable stream port named "Echo" before running this demo.
local name = "Echo"
local timeout = 1000
local port = Port.get(name)

-- Discover configured ports and inspect the selected port.
print(Port.list())
print(port:info())

port:open()
port:clear()

-- readUntil includes the delimiter in the returned data.
local line = "Hello from UniComm\r\n"
port:write(line)
print(port:readUntil("\r\n", timeout))

-- read with a positive length waits for exactly that many bytes.
local request = string.fromHex("01 03 00 00 00 01")
port:write(request)
local response = port:read(#request, timeout)
---@cast response string
print(string.toHex(response, " "))

-- port:read() immediately drains all bytes currently in the receive buffer.

-- TCP, SSL, and WebSocket servers identify a peer as "address:port".
-- Build the identifier from peerAddress and peerPort returned by port:info().
-- Omitting peerIp from port:write broadcasts; omitting it from port:read selects
-- one unspecified connected peer.
-- local server = Port.get("Server")
-- local information = server:info()
-- local peer = information.peer1
-- local peerIp = peer.peerAddress .. ":" .. peer.peerPort
-- server:write("Hello", peerIp)
-- local data = server:read(0, 0, peerIp)

port:close()

-- Remove a port created by Lua when it is no longer needed.
-- Port.remove(name)

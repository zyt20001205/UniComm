-- Start port/tcpServer first, or point this client at your own TCP server.
local client = Port.create({
    portType = Port.Type.TcpClient,
    portName = "TCP Client",
    remoteHost = "127.0.0.1",
    remotePort = 8000,
    logFormat = "utf-8",
})

client:open()
print(client:info())
client:write("Hello from UniComm\r\n")
print(client:readUntil("\r\n", 2000))
client:close()

-- To reuse this saved configuration on a later run:
-- local client = Port.get("TCP Client")

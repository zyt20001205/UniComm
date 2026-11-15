-- Start port/sslServer with its certificate and key before running this client.
local client = Port.create({
    portType = Port.Type.SslClient,
    portName = "SSL Client",
    remoteHost = "127.0.0.1",
    remotePort = 8443,
    logFormat = "utf-8",
})

client:open()
print(client:info())
client:write("Hello over TLS\r\n")
print(client:readUntil("\r\n", 2000))
client:close()

-- To reuse this saved configuration on a later run:
-- local client = Port.get("SSL Client")

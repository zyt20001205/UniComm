-- Start port/websocketServer first. remoteHost can include a path and query.
-- On later runs, use Port.get("WebSocket Client") to reuse the saved configuration.
-- For a WSS echo server, set secure=true, use its host/path and port (usually 443).
-- WSS always verifies the server certificate; there is no skip-verification mode.
local client = Port.create({
    portType = Port.Type.WebSocketClient,
    portName = "WebSocket Client",
    remoteHost = "127.0.0.1/",
    remotePort = 8080,
    secure = false,
    -- Certificate settings are ignored for plaintext WS.
    -- Empty uses the default trust store; select a PEM CA file for a private CA.
    caCertificate = "",
    -- For client authentication, provide both PEM file URLs. The certificate
    -- chain starts with the client certificate; the private key must match it.
    certificate = "",
    privateKey = "",
    -- Only needed for a password-protected key. Saved in plaintext.
    privateKeyPassword = "",
    logFormat = "hex",
})

client:open()
print(client:info())

-- Select the outgoing message type per call: 1 = UTF-8 text, 2 = binary.
client:write("Hello from UniComm", 1)
print(client:read(0, 2000))

client:write(string.fromHex("00 FF 80 41 42"), 2)
local data = client:read(0, 2000)
---@cast data string
print(string.toHex(data, " "))

-- write(data) defaults to binary. Incoming message types and boundaries are
-- discarded, so choose read lengths and delimiters from your application protocol.
client:close()

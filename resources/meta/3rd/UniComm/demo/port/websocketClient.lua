-- Start port/websocketServer first. Use a complete ws:// or wss:// URL.
-- On later runs, use Port.get("WebSocket Client") to reuse the saved configuration.
local client = Port.create({
    portType = Port.Type.WebSocketClient,
    portName = "WebSocket Client",
    url = "ws://127.0.0.1:8080/",
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

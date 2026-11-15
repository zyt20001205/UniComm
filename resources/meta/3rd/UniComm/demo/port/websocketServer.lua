-- Run this script, then start port/websocketClient in another interpreter.
-- On later runs, use Port.get("WebSocket Server") to reuse the saved configuration.
local server = Port.create({
    portType = Port.Type.WebSocketServer,
    portName = "WebSocket Server",
    localHost = "127.0.0.1",
    localPort = 8080,
    logFormat = "hex",
})

server:open()
print(server:info())

-- Incoming message types and boundaries are discarded. Echo all received bytes
-- as binary messages to preserve their values, even when they are not valid UTF-8.
for _ = 1, 300 do
    for _, peer in pairs(server:info()) do
        if type(peer) == "table" then
            local data = server:read(0, 0, peer.peerIp)
            ---@cast data string
            if data ~= "" then
                print(peer.peerIp, string.toHex(data, " "))
                server:write(data, 2, peer.peerIp)
            end
        end
    end
    thread.sleep(100)
end

-- Text broadcast: supply message type 1 and omit peerIp.
-- server:write("Hello everyone", 1)
server:close()

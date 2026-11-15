-- Provide an existing PEM certificate and private key in the workspace.
-- Run this script, then start port/sslClient in another interpreter.
-- On later runs, use Port.get("SSL Server") to reuse the saved configuration.
local server = Port.create({
    portType = Port.Type.SslServer,
    portName = "SSL Server",
    localHost = "127.0.0.1",
    localPort = 8443,
    certificate = "server-cert.pem",
    privateKey = "server-key.pem",
    logFormat = "utf-8",
})

server:open()
print(server:info())

-- Echo each peer's bytes back to that same peer for about 30 seconds.
for _ = 1, 300 do
    for _, peer in pairs(server:info()) do
        if type(peer) == "table" then
            local peerIp = peer.peerAddress .. ":" .. peer.peerPort
            local data = server:read(0, 0, peerIp)
            ---@cast data string
            if data ~= "" then
                print(peerIp, data)
                server:write(data, peerIp)
            end
        end
    end
    thread.sleep(100)
end

server:close()

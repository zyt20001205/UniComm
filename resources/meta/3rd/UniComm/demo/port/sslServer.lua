-- Set the file URLs of your existing PEM certificate and private key below.
-- Run this script, then start port/sslClient in another interpreter.
-- On later runs, use Port.get("SSL Server") to reuse the saved configuration.
local server = Port.create({
    portType = Port.Type.SslServer,
    portName = "SSL Server",
    localHost = "127.0.0.1",
    localPort = 8443,
    certificate = "file:///C:/certs/server-cert.pem",
    privateKey = "file:///C:/certs/server-key.pem",
    -- Only needed for a password-protected key. Saved in plaintext.
    privateKeyPassword = "",
    -- Empty allows clients without certificates. To require client authentication,
    -- set the file URL of your client CA bundle; only this bundle is trusted.
    -- The client must then provide its own certificate and matching private key.
    caCertificate = "",
    logFormat = "utf-8",
})

server:open()
print(server:info())

-- Echo each peer's bytes back to that same peer for about 30 seconds.
for _ = 1, 300 do
    for _, peer in pairs(server:info()) do
        if type(peer) == "table" then
            local peerIp = peer.peerIp
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

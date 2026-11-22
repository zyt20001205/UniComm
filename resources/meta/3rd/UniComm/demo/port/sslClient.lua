-- Start port/sslServer with its certificate and key before running this client.
-- This local echo example skips certificate verification. For verification,
-- use secure=true and supply the server's CA as a file URL when it is not publicly trusted.
local client = Port.create({
    portType = Port.Type.SslClient,
    portName = "SSL Client",
    remoteHost = "127.0.0.1",
    remotePort = 8443,
    -- False keeps TLS encryption but ignores all certificate settings below.
    secure = false,
    -- With secure=true, empty uses the default trust store. For a private CA,
    -- select its PEM file in Manage Port and use the resulting file URL here.
    caCertificate = "",
    -- For client authentication, provide both PEM file URLs. The certificate
    -- chain starts with the client certificate; the key must match it.
    certificate = "",
    privateKey = "",
    -- Only needed for a password-protected private key. Saved in plaintext.
    privateKeyPassword = "",
    logFormat = "utf-8",
})

client:open()
print(client:info())
client:write("Hello over TLS\r\n")
print(client:readUntil("\r\n", 2000))
client:close()

-- To reuse this saved configuration on a later run:
-- local client = Port.get("SSL Client")

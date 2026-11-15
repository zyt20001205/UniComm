-- Point remoteHost/remotePort at a UDP echo service before running.
-- On later runs, use Port.get("UDP Socket") to reuse the saved configuration.
local udp = Port.create({
    portType = Port.Type.UdpSocket,
    portName = "UDP Socket",
    localHost = "127.0.0.1",
    localPort = 0,
    remoteHost = "127.0.0.1",
    remotePort = 9000,
    logFormat = "utf-8",
})

udp:open()
-- localPort = 0 lets the system choose a local port; info reports the actual value.
print("Local port:", udp:info().localPort)
udp:write("Hello from UniComm")
print(udp:read(0, 2000))
udp:close()

-- Received datagrams are appended to a byte buffer; datagram boundaries are lost.

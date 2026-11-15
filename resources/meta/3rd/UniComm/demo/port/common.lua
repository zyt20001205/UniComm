-- Configure an echo-capable serial, UDP, TCP/SSL client or WebSocket client
-- named "Echo" before running. Use txSuffix = "null" for this example.
print("Configured ports:", Port.list())
local port = Port.get("Echo")
port:open()
print(port:info())

-- Discard old data, send a line, then inspect it without consuming it.
port:clear()
port:write("Hello from UniComm\r\n")
local preview = port:peekUntil("\r\n", 1000)
print("Preview:", preview)

-- readUntil consumes the line and includes its delimiter.
local line = port:readUntil("\r\n", 1000)
print("Received:", line)

-- A positive length waits for that many bytes; zero reads whatever is available.
-- An unsatisfied read returns an empty string after the timeout.
-- local data = port:read(8, 1000)
-- local available = port:read(0, 1000)
-- local preview = port:peek(0, 1000)

port:close()

-- Closing keeps the configuration. Remove only ports you no longer need:
-- Port.remove("Echo")

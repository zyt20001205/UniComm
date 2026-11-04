-- Configure a port named "BambuA1" before running.
-- The MQTT wrapper uses BasePort; the port configuration selects the transport.
-- For a Bambu printer, configure an SSL client to the printer's IP on port 8883.
local name = "BambuA1"
local mqtt = Mqtt.new(name, 30000)

-- Replace ACCESS_CODE with the printer's LAN access code.
-- Use a client ID that is not shared with another connected client.
-- Defaults: keepAlive = 60 seconds, cleanSession = true.
-- This call waits for CONNACK and raises an error if the broker rejects it.
mqtt:connect("UniCommA1", "bblp", "ACCESS_CODE")
print("MQTT CONNECT accepted")

-- Replace SERIAL_NUMBER with the printer's serial number.
-- Use AtMostOnce (QoS 0): consume() currently supports QoS 0 publications only.
-- subscribe() waits for the matching SUBACK and returns the granted QoS.
local topicFilter = "device/SERIAL_NUMBER/report"
local qos = mqtt:subscribe(topicFilter, Mqtt.QoS.AtMostOnce)
print("MQTT SUBACK accepted:", topicFilter, "QoS:", qos)

-- QoS 0 publishing on a generic MQTT broker (not a printer command):
-- mqtt:publish("unicomm/demo", "Hello from UniComm", Mqtt.QoS.AtMostOnce)
-- Binary and empty payloads are also allowed; retain defaults to false.
-- mqtt:publish("unicomm/demo/binary", "\0\1\255", Mqtt.QoS.AtMostOnce, false)
-- mqtt:publish("unicomm/demo/empty", "", Mqtt.QoS.AtMostOnce, false)
-- For the printer, use device/SERIAL_NUMBER/request with its protocol-specific
-- request payload. Publishing does not wait for the application reply.

-- Consume a queued publication first, or wait up to the instance's 30-second timeout.
-- The payload is binary-safe; consume() does not assume that it contains JSON.
local message = mqtt:consume()
if message then
    print("MQTT message topic:", message.topic)
    print("Payload bytes:", #message.payload)
    print(message.payload)
    -- For a topic known to carry JSON, parse it explicitly:
    -- local report = string.fromJson(message.payload)
    -- print(report)
else
    print("No complete MQTT message within the configured timeout")
end

-- consume() has no per-call timeout; it uses the timeout configured by Mqtt.new.
-- Configuring 0 makes reads nonblocking; -1 waits indefinitely. Both settings
-- also affect CONNECT/SUBACK waits. Stop requests are not checked while a read
-- is blocked, so choose a finite instance timeout when the script needs control.
-- Receiving does not send heartbeats. Long-running sessions still need keep-alive
-- traffic; automatic MQTT heartbeats are not implemented.

-- An anonymous broker can use mqtt:connect("UniCommTest") instead.
-- To request a persistent session, pass 60, false as the last two arguments.

-- Send DISCONNECT and close the transport; no MQTT reply is expected.
-- Before retrying a failed CONNECT, close and reopen the port as well.
mqtt:disconnect()

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
-- Use AtMostOnce (QoS 0) for this subscription acknowledgement test.
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

-- An anonymous broker can use mqtt:connect("UniCommTest") instead.
-- To request a persistent session, pass 60, false as the last two arguments.

-- This test does not consume queued messages or run automatic heartbeats.
-- Close the transport after CONNECT and SUBACK; no MQTT DISCONNECT is sent.
-- Before retrying a failed CONNECT, close and reopen the port as well.
Port.get(name):close()

-- Configure a port named "BambuA1" before running this creation-only demo.
-- The MQTT wrapper uses BasePort; the port configuration selects the transport.
-- For a Bambu printer, configure an SSL client to the printer's IP on port 8883.
local name = "BambuA1"
local mqtt = Mqtt.new(name, 30000)

-- Only the underlying transport is open at this stage; MQTT CONNECT comes next.
print("MQTT instance created:", mqtt ~= nil)

-- Close the transport after this creation-only test.
Port.get(name):close()

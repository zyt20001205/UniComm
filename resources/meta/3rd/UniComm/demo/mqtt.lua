-- Configure a TCP or SSL client port named "MQTT" for your broker before running.
-- Use a broker that permits subscribing and publishing to unicomm/demo.
local mqtt = Mqtt.new("MQTT", 30000)

-- Use a client ID unique among clients connected to this broker.
mqtt:connect("UniCommDemo")
-- For a broker requiring credentials, use this instead:
-- mqtt:connect("UniCommDemo", "username", "password")

local topic = "unicomm/demo"
local qos = mqtt:subscribe(topic, 0)
print("Subscribed:", topic, "QoS:", qos)

-- Publish to the same topic so this subscription can receive the message.
mqtt:publish(topic, "Hello from UniComm", 0)

-- Wait up to the timeout passed to Mqtt.new for one publication.
local message = mqtt:consume()
if message then
    print("Topic:", message.topic)
    print("Payload:", message.payload)
    -- For topics carrying JSON, decode it explicitly:
    -- local report = string.fromJson(message.payload)
else
    print("No message received")
end

-- Payloads are binary-safe.
-- mqtt:publish(topic, string.fromHex("00 01 FF"), 0)

-- Automatic keep-alive traffic is not implemented; this is a short session.
mqtt:disconnect()

---@meta

---@class Mqtt
Mqtt = {}

---
---Create an MQTT instance over an existing port and open its transport.
---
---[MQTT demo](../demo/mqtt.lua)
---
---The port configuration determines the transport. This step does not send an MQTT CONNECT packet.
---The configured timeout is shared by CONNECT, SUBSCRIBE and consume calls.
---`0` makes reads nonblocking; `-1` waits indefinitely. These settings also affect control replies.
---
---@param name portName Target port name.
---@param timeout? integer (default: 30000) Maximum time in **milliseconds** to wait for MQTT responses.
---@return mqtt
function Mqtt.new(name, timeout) end

---@enum MqttQoS
Mqtt.QoS = {
    AtMostOnce = 0,
    AtLeastOnce = 1,
    ExactlyOnce = 2,
}

---@class (exact) MqttMessage
---@field topic string MQTT topic name encoded as UTF-8.
---@field payload string Raw message payload. Binary data and empty strings are preserved.

---@class mqtt
mqtt = {}

---
---Send an MQTT 3.1.1 CONNECT packet and wait for the broker's CONNACK.
---
---Returns normally only when the connection is accepted. Timeout, malformed replies and rejected connections raise a Lua error with the reason.
---Send CONNECT only once per transport connection. Close and reopen the port before connecting again.
---The keep-alive interval is advertised to the broker; this method does not start automatic heartbeats.
---
---@param clientId string Client identifier, unique among clients connected to the same broker.
---@param username? string Omit to leave the username field absent. An empty string is an explicit empty username.
---@param password? password Omit to leave the password field absent. Requires a username when provided.
---@param keepAlive? integer (default: 60) Keep-alive interval in **seconds**, from 0 to 65535. `0` disables keep-alive.
---@param cleanSession? boolean (default: true) Start a fresh session when true; request a persistent session when false.
---@return nil
function mqtt:connect(clientId, username, password, keepAlive, cleanSession) end

---
---Subscribe to one topic filter and wait for the matching SUBACK.
---
---Returns the granted maximum delivery QoS, which may be lower than requested. Broker rejection, timeout and malformed replies raise a Lua error.
---Incoming PUBLISH packets received while waiting are queued. This method does not start background receiving or acknowledge incoming QoS 1/2 deliveries.
---Wildcard syntax is checked by the broker; an invalid filter may cause it to close the connection.
---
---@param topicFilter string Non-empty UTF-8 topic filter, up to 65535 bytes. May contain `+` and `#` wildcards.
---@param qos? MqttQoS (default: Mqtt.QoS.AtMostOnce) Maximum delivery QoS requested for this subscription.
---@return MqttQoS qos Granted maximum delivery QoS.
function mqtt:subscribe(topicFilter, qos) end

---
---Publish a message with QoS 0 without waiting for an MQTT acknowledgement.
---
---The payload is sent unchanged, including null bytes; it may be empty. No JSON or text encoding is applied.
---Only `Mqtt.QoS.AtMostOnce` is supported. Other QoS values raise a Lua error before sending.
---A normal return means the transport write succeeded, not that the broker or receiving application has processed the message.
---
---@param topic string Non-empty UTF-8 topic name, up to 65535 bytes. Topic names must not contain `+` or `#` wildcards.
---@param payload string Raw message payload, which may contain binary data.
---@param qos? MqttQoS (default: Mqtt.QoS.AtMostOnce) Delivery QoS. Only `AtMostOnce` is supported.
---@param retain? boolean (default: false) Ask the broker to retain the message for future subscribers. Keep false for command topics.
---@return nil
function mqtt:publish(topic, payload, qos, retain) end

---
---Consume the next QoS 0 publication without sending an MQTT request.
---
---Publications queued while waiting for a control reply are returned first, in arrival order.
---Otherwise, read one complete packet from the port. If no complete packet is available
---before the instance timeout configured by `Mqtt.new`, return nil and leave any incomplete packet buffered.
---An empty payload is a valid message and does not cause a nil return.
---
---Only `Mqtt.QoS.AtMostOnce` is supported. Incoming QoS 1/2 publications, malformed
---messages and unexpected control packets raise a Lua error. Subscribe with QoS 0.
---The payload is returned unchanged; decode JSON or other formats explicitly in the caller.
---
---This method does not start background receiving or send automatic heartbeats.
---Do not read from the raw port or run another MQTT operation concurrently on this connection.
---Transport reads that return no data follow the port's existing error logging behavior
---and may also result in nil. Blocking reads do not check script stop requests;
---use a finite timeout when a bounded wait is required.
---
---@return MqttMessage|nil message Next publication, or nil when no complete message is available.
function mqtt:consume() end

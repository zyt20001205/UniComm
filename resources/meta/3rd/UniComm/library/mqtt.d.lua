---@meta

---@class Mqtt
Mqtt = {}

---
---Create an MQTT instance over an existing port and open its transport.
---
---[MQTT demo](../demo/mqtt.lua)
---
---The port configuration determines the transport. This step does not send an MQTT CONNECT packet.
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

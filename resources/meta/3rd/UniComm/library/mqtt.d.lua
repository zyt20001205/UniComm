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

---@class mqtt
mqtt = {}

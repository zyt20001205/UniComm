---@meta

---@class Port
Port = {}

---
---Access configured communication ports through a common I/O interface.
---
---[Port demo](../demo/port.lua)
---

---@enum PortType
Port.Type = {
    SerialPort = 0,
    TcpClient = 2,
    TcpServer = 3,
    SslClient = 4,
    SslServer = 5,
    WebSocketClient = 6,
    WebSocketServer = 7,
    UdpSocket = 8,
    BluetoothLe = 9,
}

---@alias PortLogFormat
---| "raw"
---| "hex"
---| "ascii"
---| "utf-8"

---@alias PortSuffix
---| "null"
---| "crlf"
---| "modbus crc"
---| "modbus lrc"

---@alias WebSocketMessageType
---| 1 # Text message. Data must be valid UTF-8 after applying the configured suffix.
---| 2 # Binary message. All byte values are preserved.

---@alias BluetoothLeMode
---| 1 # Write Request: waits for the GATT write response, not an application-level reply.
---| 2 # Write Command: does not request a GATT write response.
---| 3 # Notify: subscribes to characteristic updates without ATT confirmations.
---| 4 # Indicate: subscribes to characteristic updates with confirmations handled by the Bluetooth stack.

---@class PortConfig
---@field portType PortType Port type selected from `Port.Type`.
---@field portName string Unique name used to retrieve the configured port with `Port.get`.

---@class VisionRecognitionItem
---@field x integer Horizontal coordinate in the processed frame.
---@field y integer Vertical coordinate in the processed frame.
---@field width integer Recognition width. Zero with `height` zero represents a point.
---@field height integer Recognition height. Zero with `width` zero represents a point.
---@field text? string OCR text inside this area.
---@field confidence? number OCR confidence from 0 to 100.

---@class VisionResult
---@field width integer Processed frame width.
---@field height integer Processed frame height.
---@field text string Complete recognition text, or an empty string when nothing was recognized.
---@field items VisionRecognitionItem[] Structured recognition areas or points.

---@class SerialPortConfig : PortConfig
---@field deviceName string System serial device name, such as "COM4" or "ttyUSB0". Independent of the logical `portName`.
---@field baudRate? integer (default: 115200) Baud rate from 1 to 5000000.
---@field dataBits? 5|6|7|8 (default: 8)
---@field parity? 0|2|3|4|5 (default: 0) 0=none, 2=even, 3=odd, 4=space, 5=mark.
---@field stopBits? 1|2|3 (default: 1) 1=one, 2=two, 3=one and a half.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class TcpClientPortConfig : PortConfig
---@field remoteHost string Remote hostname or IP address.
---@field remotePort integer Remote port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class TcpServerPortConfig : PortConfig
---@field localHost string Local address on which the server listens.
---@field localPort integer Local listening port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class SslClientPortConfig : PortConfig
---@field remoteHost string Remote hostname or IP address.
---@field remotePort integer Remote port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class SslServerPortConfig : PortConfig
---@field localHost string Local address on which the server listens.
---@field localPort integer Local listening port from 1 to 65535.
---@field certificate string Local path to the PEM certificate.
---@field privateKey string Local path to the PEM private key.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class WebSocketClientPortConfig : PortConfig
---@field url string Complete ws:// or wss:// URL, including any required path and query.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class WebSocketServerPortConfig : PortConfig
---@field localHost string Local IP address on which the server listens.
---@field localPort integer Local listening port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class UdpSocketPortConfig : PortConfig
---@field localHost string Local IP address to bind.
---@field localPort integer Local port from 0 to 65535. Use 0 to let the system choose a port; `port:info().localPort` reports the actual port after opening.
---@field remoteHost string Remote hostname or IP address.
---@field remotePort integer Remote port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class BluetoothLePortConfig : PortConfig
---@field adapterAddress string Local Bluetooth adapter address recorded by discovery.
---@field peripheralAddress string Device address shown by Bluetooth discovery. No Service or Characteristic UUID is required to open a connection.
---@field logFormat? PortLogFormat (default: "utf-8")
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Capacity of each subscribed characteristic's receive buffer, from 1 to 67108864 bytes.

---@alias PortCreateConfig
---| SerialPortConfig
---| TcpClientPortConfig
---| TcpServerPortConfig
---| SslClientPortConfig
---| SslServerPortConfig
---| WebSocketClientPortConfig
---| WebSocketServerPortConfig
---| UdpSocketPortConfig
---| BluetoothLePortConfig

---
---Returns the names of all configured ports.
---
---The order is unspecified.
---
---@return portName[] names
function Port.list() end

---
---Creates a configured port.
---
---Select the type with `Port.Type`, provide the fields of its matching config
---class, then operate on the returned port instance. The same validation as the
---graphical port editor is applied; invalid or duplicate configurations raise a
---Lua error.
---@param config PortCreateConfig Port configuration matching the selected `Port.Type`.
---@return port instance
function Port.create(config) end

---
---Returns an instance bound to an existing configured port.
---
---@param name portName Target port name.
---@return port instance
function Port.get(name) end

---
---Removes a configured port.
---
---The port name must exist. Invalid removal requests raise a Lua error.
---@param name portName Target port name.
---@return nil
function Port.remove(name) end

---@class port
port = {}

---
---Returns a current information snapshot for this port.
---
---Available fields depend on the port type and may include status, addresses,
---buffer usage, and connected server peers.
---WebSocket clients also return the Qt-reported `closeCode` and `closeReason`.
---WebSocket server peers include `peerIp`, ready to pass to write/read methods.
---Bluetooth LE ports return `services[serviceUuid][characteristicUuid]` capability arrays:
---`read`, `write_request`, `write_command`, `notify`, and `indicate`.
---Active subscriptions appear in `subscriptions[serviceUuid][characteristicUuid]`,
---with integer `mode`, string `bufferSize`, and string `bufferUsed` fields.
---Top-level `bufferUsed` is the sum across subscriptions; `bufferSize` is the capacity per characteristic.
---The UUID keys are lowercase; both tables are empty after disconnecting.
---@return table<string, any> information
function port:info() end

---
---Opens or starts this port.
---
---Single-buffer byte ports clear old receive data before a fresh open.
---Calling open again on an already active port does not clear data.
---
---A Vision port waits up to 30 seconds for its first valid frame.
---A Bluetooth LE port first matches existing scan results, paired devices, or connected devices.
---Otherwise, it scans for up to 5 seconds and stops as soon as the configured device is found.
---It then connects synchronously and caches the Service/Characteristic capabilities for `info()`.
---It does not explicitly request pairing or subscribe to notifications.
---Use `subscribe` explicitly to start receiving Notify or Indicate updates.
---
---@return nil
function port:open() end

---
---Closes or stops this port.
---
---Serial, UDP, TCP/SSL clients and WebSocket clients preserve bytes already
---held in the receive buffer after closing or disconnecting. These bytes remain
---readable and peekable until consumed, explicitly cleared, or a fresh open.
---Server ports keep their existing behavior and discard disconnected peer buffers.
---Bluetooth LE releases its GATT connection without removing the system pairing.
---All Bluetooth LE subscriptions and their buffered data are discarded. Subscribe again after reconnecting.
---
---@return nil
function port:close() end

---
---Discards data currently held in this port's receive buffer.
---
---For a server port, the receive buffers of all connected peers are cleared.
---For Bluetooth LE, every subscribed characteristic's buffer is cleared without ending its subscription.
---
---@return nil
function port:clear() end

---
---Writes binary-safe data to this port.
---
---TCP and SSL servers use `write(data, peerIp?)`. Omitting `peerIp` broadcasts
---to all connected peers. Other byte-stream port types ignore `peerIp`.
---
---WebSocket clients and servers use `write(data, messageType?, peerIp?)`.
---`messageType` is an integer: 1=text, 2=binary (default). Each write sends one
---message, and the type can change on every call. To target a WebSocket server
---peer, pass an explicit type followed by its `address:port` identifier as the
---third argument. Omitting `peerIp` broadcasts; clients ignore it.
---Strings such as "text" are not message types; use the integer 1 or 2.
---A successful write queues data locally; it does not confirm peer receipt.
---
---Received WebSocket text is encoded as UTF-8 and binary is preserved as bytes.
---Both enter the same receive buffer; message boundaries and types are discarded.
---
---[WebSocket loopback demo](../demo/websocket.lua)
---
---Bluetooth LE ports use `write(data, mode, serviceUuid, characteristicUuid)`.
---The generic `write(data)` and WebSocket write forms are rejected on Bluetooth LE ports.
---All four arguments are required. `mode` is 1 for Write Request or 2 for
---Write Command; unsupported modes are rejected rather than switched automatically.
---UUIDs follow the same normalization rules as the explicit UUID read overload.
---The device must be connected, and the characteristic must belong to the service
---and support the selected write capability. Invalid arguments and device errors
---raise a Lua error without deliberately closing the connection.
---Data is sent unchanged, including null bytes. The configured suffix is not
---appended, and data is not split into multiple characteristic writes.
---A Write Request response is a GATT acknowledgement, not a device application reply.
---Write Command success does not confirm that the peer received or processed data.
---
---A Vision port accepts the following commands. ROI indexes are 1-based,
---relative paths are resolved from the workspace, and paths containing spaces
---must be enclosed in double quotes. Each command waits up to 30 seconds for the
---next captured frame:
---* `raw [path]` saves the complete source frame. The default is `raw.png`.
---* `roi <index> [path]` saves the selected ROI. The default is `roi-<index>.png`.
---* `processed <index> [path]` saves the selected ROI after its pipeline. The
---  default is `processed-<index>.png`.
---* `bundle [directory]` saves the raw frame, every ROI, every processed frame,
---  and `result.json`. The default directory is `bundle`.
---
---@param data string Data to write, or a Vision export command.
---@param peerIp? string Server peer identifier in `address:port` form.
---@overload fun(self: port, data: string, messageType: WebSocketMessageType, peerIp?: string)
---@overload fun(self: port, data: string, mode: BluetoothLeMode, serviceUuid: uuid, characteristicUuid: uuid)
---@return nil
function port:write(data, peerIp) end

---
---Subscribes to updates from one Bluetooth LE characteristic.
---
---All three arguments are required. `mode` must be 3 (Notify) or 4 (Indicate),
---and the characteristic must support that capability. UUIDs use the same rules as `read`.
---A separate buffer is allocated for this Service/Characteristic pair using the configured `bufferSize`.
---Callbacks append binary-safe bytes; notification boundaries are discarded, but different characteristics never share a buffer.
---The subscription call does not wait for the first update. Some devices require a documented write command before reporting data.
---Repeating the same subscription preserves its buffer. To change modes, unsubscribe first.
---Indicate confirmations are handled below Lua and do not wait for `read`.
---A failed subscription discards its new buffer. Buffer overflow logs an error, discards the affected buffer,
---and attempts to unsubscribe that characteristic only; other subscriptions and the BLE connection remain active.
---Invalid modes, UUIDs, unsupported capabilities, and device errors raise a Lua error.
---
---@param mode BluetoothLeMode Subscription mode: 3 for Notify or 4 for Indicate.
---@param serviceUuid uuid Service containing the characteristic.
---@param characteristicUuid uuid Characteristic to subscribe to.
---@return nil
function port:subscribe(mode, serviceUuid, characteristicUuid) end

---
---Ends one Bluetooth LE characteristic's subscription and discards its buffer.
---
---Other characteristics are unaffected. Repeating this call for a known, unsubscribed
---characteristic while connected does nothing. A waiting buffer read returns an empty
---string when its subscription ends; late callbacks are ignored.
---The local buffer is discarded even if the device reports an unsubscribe error.
---Invalid UUIDs, unknown characteristics, disconnected ports, non-BLE ports, and device errors raise a Lua error.
---
---@param serviceUuid uuid Service containing the characteristic.
---@param characteristicUuid uuid Characteristic to unsubscribe from.
---@return nil
function port:unsubscribe(serviceUuid, characteristicUuid) end

---
---Reads data from this port's receive buffer.
---
---For byte-buffered ports, a positive `length` waits for exactly that many bytes.
---A zero or omitted `length` drains all currently buffered bytes. If the buffer
---is empty, it waits until any bytes arrive or the timeout expires. An expired
---timeout returns an empty string if the read cannot be satisfied. The default
---timeout is zero, so `read()` does not wait.
---
---On the single-buffer byte ports described by `close`, cached data remains
---readable after the transport closes. If the requested length cannot be satisfied,
---return an empty string immediately without consuming the tail. Use `read(0)`
---to retrieve a tail shorter than the requested length.
---
---For TCP, SSL, and WebSocket server ports, omitting `peerIp` selects one
---unspecified connected peer. A Vision port waits for the next captured frame
---according to `timeout`, then returns one structured result per configured ROI
---and always preserves the outer array. When no ROI is configured, the complete
---source frame is processed as one ROI. With `timeout` zero it uses the current
---frame immediately.
---
---Bluetooth LE ports accept `read(serviceUuid, characteristicUuid)` to request
---a characteristic value synchronously. No mode is required. UUID strings accept 4 or 8 hexadecimal
---digits, expanded using the Bluetooth Base UUID, or a full 128-bit UUID.
---The all-zero 128-bit UUID is rejected.
---Uppercase letters and surrounding braces on full UUIDs are accepted. The device must
---be connected, and the characteristic must belong to the service and support reading.
---This overload returns a binary-safe string without consuming or appending to the
---receive buffer. Its timeout is controlled by SimpleBLE and the operating system.
---Invalid UUIDs, non-BLE ports, unsupported reads, and device errors raise a Lua error.
---
---Use `read(length, timeout, serviceUuid, characteristicUuid)` to consume a subscribed
---characteristic's notification buffer instead. All four arguments are required.
---The same length and timeout rules apply; reads may split or combine notifications.
---A known characteristic without an active subscription returns an empty string immediately.
---Unsubscribe or disconnect interrupts an existing wait and discards that subscription's data.
---Unknown services or characteristics, and reads started after disconnecting, raise a Lua error.
---The generic buffer read form without both UUIDs is rejected on BLE ports.
---
---@param length? integer (default: 0) Number of bytes to read.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
---@overload fun(self: port, serviceUuid: uuid, characteristicUuid: uuid): string
---@overload fun(self: port, length: integer, timeout: integer, serviceUuid: uuid, characteristicUuid: uuid): string
---@return string|VisionResult[] data
function port:read(length, timeout, peerIp) end

---
---Peeks at data in this port's receive buffer without consuming it.
---
---The wait rules are the same as for `read`. A positive `length` waits for exactly
---that many bytes. A zero or omitted `length` returns all currently buffered bytes,
---waiting for any bytes only if the buffer is empty. An expired timeout returns
---an empty string if the peek cannot be satisfied. The default timeout is zero,
---so `peek()` does not wait. The returned string preserves all bytes, including null bytes.
---
---On single-buffer byte ports, cached data remains peekable after the transport
---closes. An incomplete request returns an empty string immediately without
---waiting or consuming buffered data.
---
---TCP, SSL, and WebSocket servers select one unspecified connected peer when
---`peerIp` is omitted. Use the same explicit `peerIp` for a peek and its later
---read. Other byte-buffered port types ignore `peerIp`. Vision ports do not
---support peeking and raise a Lua error.
---
---Peeking does not reserve data for a later read. Avoid independent consumers
---on the same connection.
---Bluetooth LE requires `peek(length, timeout, serviceUuid, characteristicUuid)`
---and follows the subscription-buffer rules described by `read`.
---
---@param length? integer (default: 0) Number of bytes to peek at.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
---@overload fun(self: port, length: integer, timeout: integer, serviceUuid: uuid, characteristicUuid: uuid): string
---@return string data
function port:peek(length, timeout, peerIp) end

---
---Reads through the first occurrence of a delimiter.
---
---The returned data includes the delimiter. If the delimiter is not found before
---the timeout expires, an empty string is returned.
---
---On single-buffer byte ports, complete buffered lines remain readable after
---the transport closes. If no delimiter is buffered, return an empty string
---immediately without consuming the tail. Use `read(0)` to retrieve that tail.
---
---For TCP, SSL, and WebSocket server ports, omitting `peerIp` selects one
---unspecified connected peer. Vision ports do not support delimiter reads.
---Bluetooth LE requires `readUntil(text, timeout, serviceUuid, characteristicUuid)`
---and reads only that subscribed characteristic's buffer. Unsuccessful reads preserve buffered bytes.
---
---@param text? string (default: "\r\n") Delimiter to read through.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
---@overload fun(self: port, text: string, timeout: integer, serviceUuid: uuid, characteristicUuid: uuid): string
---@return string data
function port:readUntil(text, timeout, peerIp) end

---
---Peeks through the first occurrence of a delimiter without consuming data.
---
---The returned string includes the delimiter and preserves all bytes, including
---null bytes. If the delimiter is not found before the timeout expires, an empty
---string is returned and the buffered data is preserved. An empty delimiter
---returns an empty string immediately.
---
---On single-buffer byte ports, complete buffered lines remain peekable after
---the transport closes. If no delimiter is buffered, return an empty string
---immediately without waiting or consuming the tail.
---
---TCP, SSL, and WebSocket servers select one unspecified connected peer when
---`peerIp` is omitted. Use the same explicit `peerIp` for a peek and its later
---read. Other byte-buffered port types ignore `peerIp`. Vision ports do not
---support peeking and raise a Lua error.
---
---Peeking does not reserve data for a later read.
---Avoid independent consumers on the same connection.
---Bluetooth LE requires `peekUntil(text, timeout, serviceUuid, characteristicUuid)`
---and follows the same subscription-buffer rules as `readUntil`, without consuming bytes.
---
---@param text? string (default: "\r\n") Delimiter to peek through.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
---@overload fun(self: port, text: string, timeout: integer, serviceUuid: uuid, characteristicUuid: uuid): string
---@return string data
function port:peekUntil(text, timeout, peerIp) end

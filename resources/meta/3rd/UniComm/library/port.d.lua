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
    Vision = 1,
    TcpClient = 2,
    TcpServer = 3,
    SslClient = 4,
    SslServer = 5,
    UdpSocket = 8,
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
---@field baudRate? integer (default: 115200) Baud rate from 1 to 5000000.
---@field dataBits? 5|6|7|8 (default: 8)
---@field parity? 0|2|3|4|5 (default: 0) 0=none, 2=even, 3=odd, 4=space, 5=mark.
---@field stopBits? 1|2|3 (default: 1) 1=one, 2=two, 3=one and a half.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@class VisionPortConfig : PortConfig
---@field portName string Exact name of an available screen or camera. Without configured ROIs, the complete source frame is processed as one ROI.

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

---@class UdpSocketPortConfig : PortConfig
---@field localHost string Local IP address to bind.
---@field localPort integer Local port from 0 to 65535. Use 0 to let the system choose a port.
---@field remoteHost string Remote hostname or IP address.
---@field remotePort integer Remote port from 1 to 65535.
---@field logFormat? PortLogFormat (default: "utf-8") Format used to render both transmitted and received data in the port log.
---@field txSuffix? PortSuffix (default: "null")
---@field bufferSize? integer (default: 65536) Receive buffer capacity in bytes, from 1 to 67108864.

---@alias PortCreateConfig
---| SerialPortConfig
---| VisionPortConfig
---| TcpClientPortConfig
---| TcpServerPortConfig
---| SslClientPortConfig
---| SslServerPortConfig
---| UdpSocketPortConfig

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
---@return table<string, any> information
function port:info() end

---
---Opens or starts this port.
---
---Single-buffer byte ports clear old receive data and reset statistics before a
---fresh open. Calling open again on an already active port does not clear data.
---
---A Vision port waits up to 30 seconds for its first valid frame.
---
---@return nil
function port:open() end

---
---Closes or stops this port.
---
---Serial, UDP, BLE, TCP/SSL clients and WebSocket clients preserve bytes already
---held in the receive buffer after closing or disconnecting. These bytes remain
---readable and peekable until consumed, explicitly cleared, or a fresh open.
---Server ports keep their existing behavior and discard disconnected peer buffers.
---
---@return nil
function port:close() end

---
---Discards data currently held in this port's receive buffer.
---
---For a server port, the receive buffers of all connected peers are cleared.
---
---@return nil
function port:clear() end

---
---Writes binary-safe data to this port.
---
---For TCP, SSL, and WebSocket server ports, omitting `peerIp` broadcasts to all
---connected peers. Other port types ignore `peerIp`.
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
---@return nil
function port:write(data, peerIp) end

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
---@param length? integer (default: 0) Number of bytes to read.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
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
---on the same connection. Reading statistics are not increased by peeking.
---
---@param length? integer (default: 0) Number of bytes to peek at.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
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
---
---@param text? string (default: "\r\n") Delimiter to read through.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
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
---Peeking does not reserve data for a later read or increase reading statistics.
---Avoid independent consumers on the same connection.
---
---@param text? string (default: "\r\n") Delimiter to peek through.
---@param timeout? integer (default: 0) Wait timeout in milliseconds: `0` returns immediately, a positive value waits up to that duration, and `-1` waits indefinitely.
---@param peerIp? string Server peer identifier in `address:port` form.
---@return string data
function port:peekUntil(text, timeout, peerIp) end

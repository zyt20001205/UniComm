---@meta

---@class Http
Http = {}

---
---Create an HTTP instance and open the configured port.
---
---[HTTP demo](../demo/http.lua)
---
---@param name portName Target port name.
---@param timeout? integer (default: 30000) Maximum time in **milliseconds** to wait for data to arrive.
---@return http
function Http.new(name, timeout) end

---@class HttpResult
---@field version string HTTP version from the status line.
---@field statusCode integer HTTP status code.
---@field reason string HTTP reason phrase.
---@field header table<string, string> Response headers with lowercase names.
---@field body string Raw response body, which may contain binary data. Empty for a HEAD response.

---@class http
http = {}

---Send an HTTP DELETE request.
---@param target string HTTP request target.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string HTTP request body.
---@return HttpResult result
function http:delete(target, header, body) end

---Send an HTTP GET request.
---@param target string HTTP request target, including any query string.
---@param header? table<string, string> Additional request header. `Host` is generated automatically; message-body framing headers are ignored.
---@return HttpResult result
function http:get(target, header) end

---Send an HTTP HEAD request.
---@param target string HTTP request target.
---@param header? table<string, string> Additional request header. `Host` is generated automatically; message-body framing headers are ignored.
---@return HttpResult result
function http:head(target, header) end

---Send an HTTP PATCH request.
---@param target string HTTP request target.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string HTTP request body.
---@return HttpResult result
function http:patch(target, header, body) end

---Send an HTTP POST request.
---@param target string HTTP request target.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string HTTP request body.
---@return HttpResult result
function http:post(target, header, body) end

---Send an HTTP PUT request.
---@param target string HTTP request target.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string HTTP request body.
---@return HttpResult result
function http:put(target, header, body) end

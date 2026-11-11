---@meta

---@class Http
Http = {}

---@alias HttpMethod
---| "GET"
---| "HEAD"
---| "POST"
---| "PUT"
---| "PATCH"
---| "DELETE"
---| "OPTIONS"
---| string

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

---@class HttpStreamResult: HttpResult
---@field body string Always empty; streamed body data is not accumulated.
---@field stopped boolean True when the callback returned false; false on normal completion.

---@class http
---Methods are case-sensitive; custom methods are accepted, except CONNECT. Protocol upgrades are unsupported.
---Do not use the same port concurrently or from a stream callback.
http = {}

---
---Send an HTTP request and collect the complete response body.
---
---@param method HttpMethod HTTP method, normally uppercase.
---@param target string HTTP request target, including any query string.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string Raw request body, which may contain binary data.
---@return HttpResult result
function http:request(method, target, header, body) end

---
---Receive body fragments synchronously on the calling Lua thread; timeout applies to each read.
---Fragments contain raw body bytes without HTTP chunk framing, not necessarily complete application messages.
---Return false to stop; nil or no return continues. Early stop or error closes the port.
---
---@param method HttpMethod HTTP method, normally uppercase.
---@param target string HTTP request target, including any query string.
---@param fn fun(data: string, response: HttpStreamResult): boolean? Called for each nonempty body fragment. Response headers are available on every call.
---@param header? table<string, string> Additional request header. `Host` and `Content-Length` are generated automatically; `Transfer-Encoding` is ignored.
---@param body? string Request body sent in full before receiving the response.
---@return HttpStreamResult result Response metadata with an empty body.
function http:stream(method, target, fn, header, body) end

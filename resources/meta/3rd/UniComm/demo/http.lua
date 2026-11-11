-- Configure a port named "HTTP" before running.
-- new() opens the configured port automatically.
local http = Http.new("HTTP", 30000)

local header = {
    ["Accept"] = "application/json",
    ["Content-Type"] = "application/json; charset=utf-8"
}

local body = [[
{
    "message": "Hello from UniComm"
}
]]

local result = http:request("POST", "/post", header, body)

print(result.statusCode)
print(result.header)
print(result.body)

---

-- Replace the target with a streaming endpoint and uncomment to try.
-- The call blocks; return false from the callback to stop and close the port.
-- local function consume(data, response)
--     print(response.statusCode, #data)
-- end
-- local streamed = http:stream("GET", "/stream", consume)
-- print(streamed.stopped)

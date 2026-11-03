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

local result = http:post("/post", header, body)

print(result.statusCode)
print(result.header)
print(result.body)

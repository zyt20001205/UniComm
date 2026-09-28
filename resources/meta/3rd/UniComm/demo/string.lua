local source = "UniComm"

-- Base64 converts arbitrary string bytes to printable text and back.
local base64 = string.toBase64(source)
print(base64)
print(string.fromBase64(base64))

-- Hex output can optionally separate each byte. fromHex accepts the separated
-- representation and restores the original string bytes.
local hex = string.toHex(source, " ")
print(hex)
print(string.fromHex(hex))

local document = string.fromJson([[
{
    "name": "UniComm",
    "enabled": true,
    "ports": ["Serial", "TCP", "Vision"]
}
]])

print(document.name)
print(document.enabled)

-- JSON arrays use Lua's one-based integer keys.
for index, portType in ipairs(document.ports) do
    print(index, portType)
end

-- Lua tables are encoded as JSON objects.
local json = string.toJson({
    name = document.name,
    enabled = document.enabled
})

print(json)

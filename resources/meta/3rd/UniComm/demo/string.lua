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

---

-- Hashes return raw bytes. Convert to hex only when displaying the digest.
local md5 = string.hash("abc", "MD5")
local sha1 = string.hash("abc", "SHA-1")
local sha256 = string.hash("abc", "SHA-256")
local sha512 = string.hash("abc", "SHA-512")
print("MD5:", string.toHex(md5))
print("SHA-1:", string.toHex(sha1))
print("SHA-256:", string.toHex(sha256))
print("SHA-512:", string.toHex(sha512))

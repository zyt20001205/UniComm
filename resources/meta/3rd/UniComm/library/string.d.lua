---@meta

---
---Converts strings between binary encodings and structured JSON values.
---JSON objects become tables with string keys, while JSON arrays become tables
---with one-based integer keys.
---JSON null becomes nil and is therefore not retained as a table entry.
---When encoding, an empty table becomes a JSON object.
---
---[String demo](../demo/string.lua)
---
string = {}

---Convert a binary string to its base64 representation.
---@param string string The binary string to convert.
---@return string
function string.toBase64(string) end

---Convert a base64 string to its binary representation.
---@param string string The base64 string to convert.
---@return string
function string.fromBase64(string) end

---Convert a binary string to its hexadecimal representation.
---@param string string The binary string to convert.
---@param separator? string Optional separator between hex bytes.
---@return string
function string.toHex(string, separator) end

---Convert a hexadecimal string to its binary representation.
---@param string string The hexadecimal string to convert.
---@return string
function string.fromHex(string) end

---Encode a Lua table as a compact JSON object.
---Numeric table keys become JSON object property names.
---@param value table Value to encode.
---@return string json Compact JSON text.
function string.toJson(value) end

---Parse a JSON object or array into a Lua table.
---@param string string JSON document to parse.
---@return table value Parsed object or array.
function string.fromJson(string) end

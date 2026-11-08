---@meta

---
---Provides binary encodings, JSON conversion, message digests, and encryption.
---JSON objects become tables with string keys, while JSON arrays become tables
---with one-based integer keys.
---JSON null becomes nil and is therefore not retained as a table entry.
---When encoding, an empty table becomes a JSON object.
---
---[String demo](../demo/string.lua)
---
string = {}

---@alias HashAlgorithm
---| "MD5"
---| "SHA-1"
---| "SHA-256"
---| "SHA-512"

---@alias CipherAlgorithm
---| "AES-128-CBC"
---| "AES-192-CBC"
---| "AES-256-CBC"

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

---
---Computes a digest of the input bytes.
---
---Returns a binary string, not hexadecimal text. Use `string.toHex` to display it.
---MD5 is intended for compatibility with legacy protocols.
---
---@param data string Input bytes.
---@param algorithm HashAlgorithm Hash algorithm.
---@return string digest Raw digest bytes.
function string.hash(data, algorithm) end

---
---Encrypts plaintext bytes with AES-CBC and PKCS#7 padding.
---
---Pass raw key and IV bytes, not hex or Base64 text. The key must be 16, 24, or
---32 bytes for AES-128, AES-192, or AES-256, respectively. CBC does not
---authenticate the ciphertext.
---
---@param data string Plaintext bytes.
---@param key password Raw AES key bytes.
---@param iv string 16 raw IV bytes.
---@param algorithm CipherAlgorithm Cipher algorithm.
---@return string ciphertext Raw ciphertext bytes.
function string.encrypt(data, key, iv, algorithm) end

---
---Decrypts ciphertext bytes and removes PKCS#7 padding.
---
---Use the same key, IV, and algorithm as for encryption. Invalid input or
---padding raises a Lua error.
---
---@param data string Ciphertext bytes.
---@param key password Raw AES key bytes.
---@param iv string 16 raw IV bytes.
---@param algorithm CipherAlgorithm Cipher algorithm.
---@return string plaintext Raw plaintext bytes.
function string.decrypt(data, key, iv, algorithm) end

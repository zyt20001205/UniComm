---@meta

---@alias portName
---| string
---| '"__PLACEHOLDER__PORTNAME__"'

---Bluetooth UUID: 4 or 8 hexadecimal digits, or a full 128-bit UUID.
---@alias uuid string

---@alias password "__PLACEHOLDER__PASSWORD__"

---@alias FileType
---| "file"
---| "directory"
---| "link"
---| "unknown"

---@class (exact) FileInfo
---@field name string Entry name.
---@field type FileType Entry type.
---@field size? integer File size in bytes.
---@field modified? string UTC modification time in `YYYYMMDDHHMMSS[.sss]` format.

--[[
---
---Show an input dialog for variable assignment.
---
---@return string
function input() end
]]

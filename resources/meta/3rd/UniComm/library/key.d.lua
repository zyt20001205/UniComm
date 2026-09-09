---@meta

---
---Sends keyboard input to the currently focused control.
---Held keys are released automatically when the Lua runtime exits.
---
---[Key demo](../demo/key.lua)
---
key = {}

---@alias KeyName
---| "0"
---| "1"
---| "2"
---| "3"
---| "4"
---| "5"
---| "6"
---| "7"
---| "8"
---| "9"
---| "A"
---| "B"
---| "C"
---| "D"
---| "E"
---| "F"
---| "G"
---| "H"
---| "I"
---| "J"
---| "K"
---| "L"
---| "M"
---| "N"
---| "O"
---| "P"
---| "Q"
---| "R"
---| "S"
---| "T"
---| "U"
---| "V"
---| "W"
---| "X"
---| "Y"
---| "Z"
---| "BACKSPACE"
---| "TAB"
---| "ENTER"
---| "SHIFT"
---| "CTRL"
---| "ALT"
---| "PAUSE"
---| "CAPSLOCK"
---| "ESC"
---| "SPACE"
---| "PAGEUP"
---| "PAGEDOWN"
---| "END"
---| "HOME"
---| "LEFT"
---| "UP"
---| "RIGHT"
---| "DOWN"
---| "PRINTSCREEN"
---| "INSERT"
---| "DELETE"

---Presses and holds a key.
---Key names are case-insensitive.
---@param name KeyName Key to press.
---@return nil
function key.down(name) end

---Releases a key previously held with `key.down`.
---Key names are case-insensitive.
---@param name KeyName Key to release.
---@return nil
function key.up(name) end

---Presses and immediately releases a key.
---Key names are case-insensitive.
---@param name KeyName Key to tap.
---@return nil
function key.tap(name) end

---Types Unicode text into the currently focused control.
---Use `key.tap` for control keys such as Enter or Tab.
---@param text string Text to type.
---@return nil
function key.type(text) end

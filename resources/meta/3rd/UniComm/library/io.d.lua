---@meta

---
---Extends the standard Lua I/O library with UniComm application operations.
---Relative paths used by `io.open`, `io.input`, `io.output`, `io.lines`,
---`os.remove`, and `os.rename` start from the current workspace.
---
---[IO demo](../demo/io.lua)
---
io = {}

---Converts text to speech and outputs it through the audio system.
---@param text string The text content to be spoken.
---@return nil
function io.speak(text) end

---@meta

---
---Displays script-owned user interfaces in independent windows.
---
---[UI demo](../demo/ui.lua)
---
ui = {}

---@alias MessageButton
---| "ok"
---| "cancel"

---
---Displays a message window and waits for the user's choice.
---
---Only the calling script waits; the application and other scripts remain usable.
---Pass nil for title to use the current threadId. The default button list is {"ok"}.
---OK or Enter returns true. Cancel, Escape, or closing the window returns false.
---Enter accepts only when an OK button is present.
---In Run/Agent mode, stopping the script closes its window and terminates the wait.
---
---@param title? string (default: current threadId) Window title. Pass nil to use the default.
---@param text string Plain text to display. Newlines are supported.
---@param button? MessageButton[] (default: {"ok"}) Non-empty list of buttons to display.
---@return boolean accepted True for OK; false for cancellation or closing the window.
function ui.message(title, text, button) end

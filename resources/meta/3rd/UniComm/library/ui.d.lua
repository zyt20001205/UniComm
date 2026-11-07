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

---@alias FileDialogMode
---| "open" # Select one existing file.
---| "save" # Choose a file path; the file does not need to exist.

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

---
---Displays a single-line input window and waits for the user's choice.
---
---Only the calling script waits; the application and other scripts remain usable.
---Pass nil for title to use the current threadId. Buttons are always OK and Cancel.
---OK or Enter in the input field returns the text, including an empty string.
---Cancel, Escape, or closing the window returns nil. No whitespace is trimmed.
---The result is always text; use tonumber or other Lua functions to convert it.
---In Run/Agent mode, stopping the script closes its window and terminates the wait.
---
---@param title? string (default: current threadId) Window title. Pass nil to use the default.
---@param text string Plain-text prompt. Newlines are supported.
---@param value? string (default: "") Initial text shown in the input field.
---@return string|nil value Confirmed text, or nil if cancelled or closed.
function ui.input(title, text, value) end

---
---Displays a file chooser and waits for the user's choice.
---
---Pass nil for title to use the current threadId. The initial directory is the
---current workspace. The platform's native file dialog is used when available.
---Open mode selects one existing file. Save mode allows a new file name and keeps
---the dialog's default overwrite confirmation for an existing target.
---Returns an absolute file path, or nil on cancellation or closing the dialog.
---This only selects a path: it does not read, create, or overwrite file contents.
---Use standard Lua io to read or write the selected path yourself.
---Only the calling script waits; GUI event processing and other scripts continue.
---In Run/Agent mode, stopping the script closes the dialog and terminates the wait.
---
---@param title? string (default: current threadId) Dialog title. Pass nil to use the default.
---@param mode? FileDialogMode (default: "open") File selection mode.
---@return string|nil path Absolute file path, or nil if cancelled or closed.
function ui.file(title, mode) end

---
---Opens an independent image window and returns without waiting for it to close.
---
---Omit source to open an empty window, then use setData for encoded image bytes or
---setPath for a file. Relative image paths start from the current workspace.
---Each call creates a separate window. Garbage-collecting its handle does not close it.
---When the script returns normally, the interpreter waits until its remaining
---image windows are closed. Stopping the script or a Lua error closes them.
---Use `local viewer <close> = ui.image(nil)` to close a window automatically
---when its scope ends, or call `viewer:close()` to close it explicitly.
---
---@param title? string (default: current threadId) Window title. Pass nil to use the default.
---@param source? string Initial image file path. Omit to create an empty window.
---@return ImageViewer viewer Handle for this image window.
function ui.image(title, source) end

---@class ImageViewer
---@field private __close fun(self: ImageViewer, error: any)
local imageViewer = {}

---
---Displays encoded image bytes directly from memory, without writing a file.
---
---Pass raw JPEG, PNG, or other Qt-supported encoded image bytes, not Base64 or
---unencoded pixel data. Binary strings containing null bytes are preserved.
---Each update replaces the previous in-memory image. Invalid image data or a
---closed window raises a Lua error. setData and setPath can be used on the same window.
---
---@param data string Raw encoded image bytes, for example an HTTP response body.
---@return nil
function imageViewer:setData(data) end

---
---Loads an image file into this window without waiting for the user to close it.
---
---Calling this again with the same path reloads the file, including overwritten
---camera snapshots. Updating a window that has been closed raises a Lua error.
---
---@param path string Image file path, absolute or relative to the current workspace.
---@return nil
function imageViewer:setPath(path) end

---
---Closes this image window. Calling close again is harmless.
---
---This is also called automatically for a `<close>` variable when its scope ends.
---
---@return nil
function imageViewer:close() end

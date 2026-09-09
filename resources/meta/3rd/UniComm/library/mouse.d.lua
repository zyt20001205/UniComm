---@meta

---
---Controls the system mouse using Qt global screen coordinates.
---Every position returned or accepted by this package uses the same coordinate
---system as `QCursor` and `QScreen.geometry`; values returned by
---`mouse.position` can be passed back without DPI conversion.
---
---[Mouse demo](../demo/mouse.lua)
---
mouse = {}

---@alias MouseButton
---| "left"
---| "right"
---| "middle"

---Returns the current mouse position.
---@return integer x Horizontal position in Qt global screen coordinates.
---@return integer y Vertical position in Qt global screen coordinates.
function mouse.position() end

---Moves the mouse immediately to the specified position.
---@param x integer | "__PLACEHOLDER__GETPOSITION__" Horizontal position in the same Qt coordinate system returned by `mouse.position`.
---@param y integer Vertical position in the same Qt coordinate system returned by `mouse.position`.
---@return nil
function mouse.move(x, y) end

---Presses and holds a mouse button at the current position.
---@param button? MouseButton (default: "left") Button to press.
---@return nil
function mouse.down(button) end

---Releases a mouse button at the current position.
---@param button? MouseButton (default: "left") Button to release.
---@return nil
function mouse.up(button) end

---Clicks a mouse button at the specified position.
---@param x integer | "__PLACEHOLDER__GETPOSITION__" Horizontal position in the same Qt coordinate system returned by `mouse.position`.
---@param y integer Vertical position in the same Qt coordinate system returned by `mouse.position`.
---@param button? MouseButton (default: "left") Button to click.
---@return nil
function mouse.click(x, y, button) end

---Double-clicks a mouse button at the specified position.
---@param x integer | "__PLACEHOLDER__GETPOSITION__" Horizontal position in the same Qt coordinate system returned by `mouse.position`.
---@param y integer Vertical position in the same Qt coordinate system returned by `mouse.position`.
---@param button? MouseButton (default: "left") Button to double-click.
---@return nil
function mouse.doubleClick(x, y, button) end

---Scrolls vertically at the specified position.
---@param x integer | "__PLACEHOLDER__GETPOSITION__" Horizontal position in the same Qt coordinate system returned by `mouse.position`.
---@param y integer Vertical position in the same Qt coordinate system returned by `mouse.position`.
---@param steps integer Number of wheel steps. Positive values scroll up and negative values scroll down.
---@return nil
function mouse.scroll(x, y, steps) end

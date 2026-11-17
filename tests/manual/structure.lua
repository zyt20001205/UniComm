-- Manual test for the Structure panel.
-- Open this file to inspect symbols, icons, colors, tooltips, and the filter menu.
-- Covers all ten symbol kinds emitted by the bundled LuaLS.
-- LSP defines more kinds, but this LuaLS document-symbol provider emits only these ten.
-- Expand and collapse the tree, then try Copy Name and Copy Detail.
-- Toggle kinds, All, or None to filter the tree immediately while the menu stays open.

-- Variable: an uninitialized local has no detail text.
local pending

-- Scalar kinds: Boolean, String, and Number.
local enabled = true
local title = "Structure panel demo"
local count = 3

-- Array: positional entries also provide child symbols.
local values = { 10, 20, 30 }

-- Object: named fields and nested tables exercise multiple tree depths.
local settings = {
    name = "Demo",
    active = true,
    retries = 2,
    labels = { "first", "second", "third" },
    display = {
        width = 640,
        height = 480,
        theme = {
            name = "Dark",
            contrast = 0.8,
        },
    },
}

-- Function: a named local function includes its signature in the detail.
-- Constant: LuaLS classifies function parameters as Constant symbols.
---@param left number
---@param right number
---@return number
local function add(left, right)
    local total = left + right
    return total
end

-- Method: colon syntax distinguishes a method from a regular function.
---@param prefix string
---@return string
function settings:describe(prefix)
    return prefix .. self.name
end

-- Package: LuaLS uses this kind for control-flow blocks.
-- Each branch contains a nested symbol for expand/collapse testing.
if enabled then
    local status = "enabled"
    settings.status = status
elseif count == 0 then
    local status = "empty"
    settings.status = status
else
    local status = "disabled"
    settings.status = status
end

-- Additional Package examples: numeric for, generic for, while, and repeat.
for index = 1, count do
    local doubled = index * 2
    values[index] = doubled
end

for index, value in ipairs(values) do
    local adjusted = add(value, index)
    values[index] = adjusted
end

local remaining = 2
while remaining > 0 do
    local nextValue = remaining - 1
    remaining = nextValue
end

repeat
    local nextValue = remaining + 1
    remaining = nextValue
until remaining == 1

-- Returning the examples keeps their local bindings in use.
return {
    pending = pending,
    enabled = enabled,
    title = title,
    count = count,
    values = values,
    settings = settings,
    add = add,
    description = settings:describe("Profile: "),
    remaining = remaining,
}

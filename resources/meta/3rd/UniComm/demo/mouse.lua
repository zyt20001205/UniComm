-- Read and preserve the current position. Every mouse API uses the same Qt
-- global screen coordinates, so these values can be passed back directly
-- without multiplying or dividing by the display scale factor.
local x, y = mouse.position()
print({ x = x, y = y })

-- Moving to the current position is safe and demonstrates the coordinate API.
mouse.move(x, y)

-- Click examples:
-- mouse.click(500, 300)
-- mouse.click(500, 300, "right")
-- mouse.doubleClick(500, 300)

-- Scroll three steps down at the target position:
-- mouse.scroll(500, 300, -3)

-- Drag from one position to another. A held button is released automatically if
-- the Lua runtime exits before mouse.up is reached.
-- mouse.move(500, 300)
-- mouse.down("left")
-- mouse.move(800, 500)
-- mouse.up("left")

-- Type text into the currently focused control, then submit it.
key.type("testKey")
key.tap("ENTER")

-- Hold a modifier while tapping another key. Held keys are released
-- automatically if the Lua runtime exits before key.up is reached.
-- key.down("CTRL")
-- key.tap("A")
-- key.up("CTRL")

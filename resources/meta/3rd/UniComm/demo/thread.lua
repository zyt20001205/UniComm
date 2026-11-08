local config = {
    [1] = "Sensor",
    ["1"] = "text key",
    count = 3,
    interval = 100
}
local payload = "\0\255"

-- The named function captures config and payload automatically.
local function sample()
    assert(config[1] == "Sensor" and config["1"] == "text key")
    assert(payload == "\0\255")

    for index = 1, config.count do
        print(config[1], index, #payload)
        thread.sleep(config.interval)
    end
end

local id = thread.start(sample)
config[1] = "Parent"
print("Started", id, "parent config:", config[1])

---

-- An inline function works too. Stop requests do not wait for completion.
local repeating = thread.start(function()
    for index = 1, 10 do
        print("Background tick", index)
        thread.sleep(250)
    end
end)
thread.sleep(500)
thread.stop(repeating)

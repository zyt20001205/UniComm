local config = {
    name = "Sensor",
    count = 3,
    interval = 100
}

-- The background function receives a copy of the captured configuration.
local function sample()
    for index = 1, config.count do
        print(config.name, index)
        thread.sleep(config.interval)
    end
end

local id = thread.start(sample)
print("Started", id)

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

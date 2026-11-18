-- Set breakpoints on the two lines marked below, then run this file in Debug mode.
-- Both workers stop at the same source line with different call stacks.
-- The main thread prints their IDs before it stops at its own breakpoint.

local function startWorker(name, depth)
    return thread.start(function()
        local function readSample()
            local reading = 20
            return reading * 2  -- Breakpoint 1: inspect reading and count the collect frames.
        end

        local function collect(remaining)
            if remaining == 0 then
                local sample = readSample()
                return sample
            end

            -- Add after the recursive call so each level stays on the stack.
            local sample = collect(remaining - 1)
            return sample + 1
        end

        local result = collect(depth)
        print(name, result)
    end)
end

local shallow = startWorker("shallow", 0)
local deep = startWorker("deep", 4)
print("shallow", shallow, "deep", deep)
print("Main finished")  -- Breakpoint 2: switch between all three threads before resuming.

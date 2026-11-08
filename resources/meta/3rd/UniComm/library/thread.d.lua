---@meta

---
---Runs Lua functions in independent interpreter threads.
---
---[Thread demo](../demo/thread.lua)
---
thread = {}

---
---Starts a Lua function in a new thread with the caller's execution mode.
---Captured local variables are copied at launch. Captures can contain nil,
---booleans, numbers, strings, and acyclic tables without metatables.
---Table keys must be strings or integers.
---The new thread has its own globals. The function's results are not returned.
---
---@param fn function The Lua function to run without arguments.
---@return string threadId The identifier of the new thread.
function thread.start(fn) end

---Requests that the specified thread stop, without waiting for it to finish.
---@param threadId string The identifier of the thread to stop.
---@return nil
function thread.stop(threadId) end

---Suspends the current thread for a specified amount of time.
---@param ms integer The number of milliseconds to sleep.
---@return nil
function thread.sleep(ms) end

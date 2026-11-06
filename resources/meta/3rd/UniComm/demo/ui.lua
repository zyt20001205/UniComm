-- Use normal Run mode. This script waits until the independent window is dismissed.
-- nil selects the current threadId as the title. Omitting button shows only OK.
print("Before message")
local accepted = ui.message(nil, "Hello from UniComm!\nOK/Enter returns true; Escape or closing returns false.")
print("Accepted:", accepted)
print("After message")

---

-- A custom title and two standard buttons. Only an explicit acceptance is true.
local confirmed = ui.message("Confirm", "Do you want to continue?", {"ok", "cancel"})
if confirmed then
    print("Confirmed")
else
    print("Cancelled")
end

---

-- Run this demo in two interpreters to verify that each gets its own window.
-- Stop one script while its window is open: only that window should close,
-- and that script must not reach the print after ui.message.
-- local result = ui.message(nil, "Stop this script while this window is open.", {"cancel"})
-- print("This line should not run after stopping the script.")

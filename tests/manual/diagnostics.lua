-- Diagnostics panel demo.
-- Open this file in UniComm; it is intentionally invalid and should not be run.
-- Expected with the bundled LuaLS defaults: one diagnostic at each severity.
-- Remove one example at a time to check counters, filters, sorting, and navigation.

-- Hint: this local variable is intentionally never read (unused-local).
local unusedHint = 42

-- Information: this global name intentionally starts with a lowercase letter
-- (lowercase-global). Adding "local" should remove this diagnostic.
diagnosticsDemoInfo = 42

-- Warning: this global variable is intentionally never defined (undefined-global).
print(DiagnosticsDemoMissingGlobal)

-- Error: a break statement is only valid inside a loop.
-- Keep this example last so the earlier examples remain easy to inspect.
break

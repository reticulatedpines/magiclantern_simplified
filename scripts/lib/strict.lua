-- strict.lua
-- checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
-- distributed under the Lua license: http://www.lua.org/license.html

-- This library is loaded automatically by ML Lua.
-- If you really need the non-strict mode, you are welcome to write
-- a library (nostrict.lua) that undoes the changes done by this file.

local getinfo, error, rawset, rawget = debug.getinfo, error, rawset, rawget

local mt = getmetatable(_G)
if mt == nil then
  mt = {}
  setmetatable(_G, mt)
end

mt.__declared = {}

local function what ()
  local d = getinfo(3, "S")
  return d and d.what or "C"
end

mt.__newindex = function (t, n, v)
  if not mt.__declared[n] then
    local w = what()
    if w ~= "main" and w ~= "C" then
      error("assign to undeclared variable '"..n.."'", 2)
    end
    mt.__declared[n] = true
  end
  rawset(t, n, v)
end

-- we have already defined __index in the global metatable,
-- in luaCB_global_index, so we want to call it as well
mt.__global_index = mt.__index

mt.__index = function (t, n)
  if not mt.__declared[n] and what() ~= "C" then
    -- global_index will either load some library and return 1,
    -- or will do nothing and return 0
    if mt.__global_index(t, n) then
      return rawget(t, n)   -- library loaded, so the requested field is now present
    end
    error("variable '"..n.."' is not declared", 2)
  end
  return rawget(t, n)
end

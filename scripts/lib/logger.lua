--[[---------------------------------------------------------------------------
Logging helper. Writes to a log file and prints to the console.

This module is a lua script (@{logger.lua}), you need to explicitly load it
with `require('logger')`

@module logger
@usage
require('logger')

local log = logger("MYLOG.LOG")
log:write("hello world!")
log:close()
]]
logger = {}
logger_metatable = {}

--[[---------------------------------------------------------------------------
Creates a new logger
@tparam string filename the filename of the log file
@tparam[opt] table listener an additional 'listener' to write data to, should implement a t:write(str) function
@treturn logger the new instance
@function logger
]]
function logger_metatable.__call(self,filename,listener)
    local t = {}
    t.filename = filename
    t.listener = listener
    t.logfile = io.open(filename, "a")
    t.logfile:setvbuf("line")
    local calling_filename = debug.getinfo(2,"S").short_src
    local date = dryos.date
    t.logfile:write(string.format([[

===============================================================================
%s - %d-%d-%d %02d:%02d:%02d
===============================================================================

]],
    tostring(calling_filename), date.year, date.month, date.day, date.hour, date.min, date.sec))
    local m = {}
    m.__index = logger
    setmetatable(t,m)
    return t
end

setmetatable(logger,logger_metatable)

--[[---------------------------------------------------------------------------
Splits a string into a table consisting of the lines of the original string
@tparam string str
@function logger.tolines
]]
function logger.tolines(str)
    local t = {}
    local p1 = 1
    local p2 = 1
    local len = #str
    while p2 <= len do
        local c = str:sub(p2,p2)
        if p2 == len then
            if p1 == p2 then table.insert(t,"")
            else table.insert(t, str:sub(p1,p2)) end
            --ends in newline?
            if c == "\r" or c == "\n" then table.insert(t,"") end
            break
        elseif c == "\r" or c == "\n" then
            if p1 == p2 then table.insert(t,"")
            else table.insert(t, str:sub(p1,p2-1)) end
            p2 = p2 + 1
            if c == "\r" and str:sub(p2,p2) == "\n" then
                p2 = p2 + 1
            end
            p1 = p2
        else
            p2 = p2 + 1
        end
    end
    return t
end

--[[---------------------------------------------------------------------------
@type logger
]]

--[[---------------------------------------------------------------------------
Writes a string to the log
@param str the string
@function write
]]
function logger:write(str)
    str = tostring(str)
    if self.listener ~= nil then self.listener:write(str) end
    io.write(str)
    self.logfile:write(str)
end

--[[---------------------------------------------------------------------------
Writes a printf style formatted string to the log
@tparam string fmt format string
@param ... format arguments
@function writef
]]
function logger:writef(fmt,...)
    self:write(fmt:format(...))
end

--[[---------------------------------------------------------------------------
Converts any Lua type to a string representation and logs it.
Recursively enumerates table structures 
@param o the object to serialize
@function serialize
]]
function logger:serialize(o,l)
    if type(o) == "string" then
        self:writef("%q",o)
    elseif type(o) == "table" or type(o) == "userdata" then
        if l == nil then l = 1 end
        --prevent infinite recursion
        if l < 10 then
            local s,e = pcall(function() pairs(o) end)
            if s == true then
                -- something iterable
                self:writef("%s:\n",type(o))
                for k,v in pairs(o) do
                    for i=1,l,1 do self:write("  ") end
                    if type(k) == "string" then self:writef("%s = ",k)
                    else self:writef("[%s] = ",tostring(k)) end
                    self:serialize(v,l+1)
                    if type(v) ~= "table" then self:write("\n") end
                end
            else
                -- something not iterable
                self:writef("%s",type(o))
            end
        end
    else
        self:write(tostring(o))
    end
end

--[[---------------------------------------------------------------------------
Close the log
@function close
]]
function logger:close()
    self.logfile:close()
end

return logger

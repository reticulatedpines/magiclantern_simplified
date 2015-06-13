--[[---------------------------------------------------------------------------
Helper for dealing with keypresses

This module is a lua script (@{keys.lua}), you need to explicitly load it
with `require('keys')`

@module keys
]]
keys = {}
keys.runnning = false

--[[---------------------------------------------------------------------------
@type keys
]]

--[[---------------------------------------------------------------------------
Starts the key handler if not already running, returns whether or not it was 
started
@function keys
]]
function keys:start()
    if keys.running then self:reset() return false end
    --save any previous keypress handler so we can restore it when finished
    self.old_keypress = event.keypress
    self.keys = {}
    self.key_count = 0
    self.running = true
    event.keypress = function(key)
        if key ~= 0 then
            keys.key_count = keys.key_count + 1
            keys.keys[keys.key_count] = key
        end
        return false
    end
    return true
end

--[[---------------------------------------------------------------------------
Returns a table of all the keys that have been pressed since the last time 
getkeys was called
@function getkeys
]]
function keys:getkeys()
    if self.key_count == 0 then 
        return nil
    else
        local result = self.keys
        self:reset()
        return result
    end
end

function keys:reset()
    self.key_count = 0
    self.keys = {}
end

--[[---------------------------------------------------------------------------
Stops the keys
@function stop
]]
function keys:stop()
    self:reset()
    self.running = false
    event.keypress = self.old_keypress
end

--[[---------------------------------------------------------------------------
Blocks until any key is pressed
@function anykey
]]
function keys:anykey()
    local started = self:start()
    --ignore any immediate keys
    task.yield(100)
    self:getkeys()
    while true do
        local keys = self:getkeys()
        if keys ~= nil then
            local exit = false
            for i,v in ipairs(keys) do
                if v ~= KEY.UNPRESS_SET then
                    exit = true
                    break
                end
            end
            if exit then break end
        end
        task.yield(100)
    end
    if started then self:stop() end
end

return keys
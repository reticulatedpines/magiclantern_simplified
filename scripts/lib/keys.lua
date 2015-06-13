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
    if keys.running then
        self:reset()
        return false
    end
    --save any previous keypress handler so we can restore it when finished
    self.old_keypress = event.keypress
    self.keys = {}
    self.running = true
    event.keypress = function(key)
        if key ~= 0 then
            table.insert(keys.keys, key)
        end
        if key <= KEY.UNPRESS_FULLSHUTTER then
            return true -- do not block half-shutter, full-shutter and unknown (non-button) events
        end
        return false    -- block regular button events
    end
    return true
end

--[[---------------------------------------------------------------------------
Returns a single key that has been pressed since the last time 
getkey was called, or nil if no key was pressed.
@function getkey
]]
function keys:getkey()
    return table.remove(self.keys, 1)
end

function keys:reset()
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
    self:reset()
    while true do
        local key = self:getkey()
        if key ~= nil then
            if key ~= KEY.UNPRESS_SET then
                break
            end
        end
        task.yield(100)
    end
    if started then self:stop() end
end

return keys

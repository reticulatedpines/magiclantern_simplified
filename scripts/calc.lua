-- a simple calculator
calc = {}
calc.value = ""
calc.buttons =
{
    "lep()^/",
    "sct789*",
    "dvy456-",
    "afg123+",
    "rq,0.C="
}
calc.row = 5
calc.col = 1
calc.rows = #(calc.buttons)
calc.cols = #(calc.buttons[1])
calc.font = FONT.LARGE
calc.pad = 20
calc.cell_size = calc.pad * 2 + calc.font.height
calc.height = (calc.rows + 1) * calc.cell_size
calc.width = (calc.cols) * calc.cell_size
calc.left = display.width // 2 - calc.width // 2
calc.top = display.height // 2 - calc.height // 2

calc.mlmenu = menu.new
{
    parent = "Debug",
    name = "Calculator",
    icon_type = ICON_TYPE.ACTION,
    select = function(this)
        task.create(function() calc:run() end)
    end,
    update = function(this) return calc.value end
}

-- The main program loop
function calc:run()
    local status, error = xpcall(function()
        menu.block(true)
        self:main_loop()
    end, debug.traceback)
    if status == false then
        handle_error(error)
    end
    menu.block(false)
    keyhandler:stop()
end

function calc:main_loop()
    menu.block(true)
    self:draw()
    keyhandler:start()
    while true do
        local keys = keyhandler:getkeys()
        if keys ~= nil then
            local exit = false
            for i,v in ipairs(keys) do
                if self:handle_key(v) == false then
                    exit = true
                    break
                end
            end
            if exit then break end
            self:draw()
        end
        task.yield(100)
    end
    keyhandler:stop()
    if self.running == false then menu.block(false) end
end

function calc:handle_key(key)
    if key == KEY.Q or key == KEY.MENU then return false
    elseif key == KEY.UP or key == KEY.WHEEL_UP then
        self.row = dec(self.row,1,self.rows)
    elseif key == KEY.DOWN or key == KEY.WHEEL_DOWN then
        self.row = inc(self.row,1,self.rows)
    elseif key == KEY.LEFT or key == KEY.WHEEL_LEFT then
        self.col = dec(self.col,1,self.cols)
    elseif key == KEY.RIGHT or key == KEY.WHEEL_RIGHT then
        self.col = inc(self.col,1,self.cols)
    elseif key == KEY.INFO then
        self:handle_button("C")
    elseif key == KEY.PLAY then
        self:handle_button("=")
    elseif key == KEY.SET then
        local row = self.buttons[self.row]
        local c = string.sub(row,self.col,self.col)
        self:handle_button(c)
    end
end

function calc:handle_button(c)
    if c == "C" then
        self.value = ""
    elseif c == "=" then
        local status,result = pcall(load,"return "..self.value)
        if status == false or result == nil then
            self.value = "syntax error"
        else
            status,self.value = pcall(result)
        end
    else
        local str = self:c_to_str(c)
        self.value = self.value..str
        if str ~= c and str ~= "pi" then
            self.value = self.value.."("
        end
    end
end

function calc:c_to_str(c)
    if c== "l" then return "log"
    elseif c == "e" then return "exp"
    elseif c == "p" then return "pi"
    elseif c == "s" then return "sin"
    elseif c == "c" then return "cos"
    elseif c == "t" then return "tan"
    elseif c == "d" then return "asin"
    elseif c == "v" then return "acos"
    elseif c == "y" then return "atan"
    elseif c == "a" then return "abs"
    elseif c == "f" then return "floor"
    elseif c == "g" then return "ceil"
    elseif c == "r" then return "rand"
    elseif c == "q" then return "sqrt"
    else return c end
end

function calc:draw()
    display.draw(function()
        display.rect(self.left,self.top,self.width,self.cell_size,COLOR.WHITE,COLOR.BLACK)
        display.print(self.value,self.left + self.pad,self.top + self.pad,self.font,COLOR.WHITE,COLOR.BLACK)
        for i=1,self.rows,1 do
            local row = self.buttons[i]
            for j=1,self.cols,1 do
                local c = self:c_to_str(string.sub(row,j,j))
                local x = self.left + (j - 1) * self.cell_size
                local y = self.top + i * self.cell_size
                local bg = COLOR.BLACK
                if i == self.row and j == self.col then bg = COLOR.BLUE end
                display.rect(x,y,self.cell_size,self.cell_size,COLOR.WHITE,bg)
                local pad = self.cell_size // 2 - self.font:width(c) // 2
                display.print(c,x + pad, y + self.pad,self.font,COLOR.WHITE,bg)
            end
        end
    end)
end

--export all the math functions to the global namespace
for k,v in pairs(math) do
    _G[k] = v
end

function inc(val,min,max)
    if val == max then return min end
    if val < min then return min end
    return val + 1
end

function dec(val,min,max)
    if val == min then return max end
    if val > max then return max end
    return val - 1
end

-- keyhandler class

keyhandler = {}
keyhandler.halfshutter = false
keyhandler.runnning = false

--starts the keyhandler if not already running, returns whether or not it was started
function keyhandler:start()
    if keyhandler.running then self:reset() return false end
    --save any previous keypress handler so we can restore it when finished
    self.old_keypress = event.keypress
    self.halfshutter = false
    self.keys = {}
    self.key_count = 0
    self.running = true
    event.keypress = function(key)
        if key == KEY.HALFSHUTTER then keyhandler.halfshutter = true end
        if key == KEY.UNPRESS_HALFSHUTTER then keyhandler.halfshutter = false end
        if key ~= 0 then
            keyhandler.key_count = keyhandler.key_count + 1
            keyhandler.keys[keyhandler.key_count] = key
        end
        return false
    end
    return true
end

-- returns a table of all the keys that have been pressed since the last time getkeys was called
function keyhandler:getkeys()
    if self.key_count == 0 then 
        return nil
    else
        local result = self.keys
        self:reset()
        return result
    end
end

function keyhandler:reset()
    self.key_count = 0
    self.keys = {}
end

function keyhandler:stop()
    self:reset()
    self.running = false
    event.keypress = self.old_keypress
end

--blocks until any key is pressed
function keyhandler:anykey()
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

function handle_error(error)
    if error == nil then error = "Unknown Error!\n" end
    local f = FONT.MONO_20
    print(error)
    display.rect(0,0,display.width,display.height,COLOR.RED,COLOR.BLACK)
    local pos = 10
    for line in error:gmatch("[^\r\n]+") do
        local clipped = display.print(line,10,pos,f)
        while clipped ~= nil do
            pos = pos + f.height
            clipped = display.print(clipped,10,pos,f)
        end
        pos = pos + f.height
    end
    keyhandler:anykey()
end

-- Calculator
-- A simple calculator in Lua

require("keys")
require("logger")

calc = {}
calc.value = ""
calc.buttons =
{
    {"log", "exp",  "pi",  "(",")","^","/"},
    {"sin", "cos",  "tan", "7","8","9","*"},
    {"asin","acos", "atan","4","5","6","-"},
    {"abs", "floor","ceil","1","2","3","+"},
    {"sqrt","%",    ",",   "0",".","C","="}
}
calc.row = 5
calc.col = 4
calc.rows = #(calc.buttons)
calc.cols = #(calc.buttons[1])
calc.font = FONT.LARGE
calc.border = COLOR.gray(75)
calc.background = COLOR.gray(5)
calc.foreground = COLOR.WHITE
calc.highlight = COLOR.BLUE
calc.error_forground = COLOR.RED
calc.pad = 20
calc.cell_size = calc.pad * 2 + calc.font.height
calc.height = (calc.rows + 1) * calc.cell_size
calc.width = (calc.cols) * calc.cell_size
calc.left = display.width // 2 - calc.width // 2
calc.top = display.height // 2 - calc.height // 2
calc.error = false

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
    keys:stop()
end

function calc:main_loop()
    menu.block(true)
    self:draw()
    keys:start()
    while true do
        if menu.visible == false then break end
        local key = keys:getkey()
        if key ~= nil then
            if self:handle_key(key) == false then
                break
            end
            self:draw()
        end
        task.yield(100)
    end
    keys:stop()
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
        self:handle_button(self.buttons[self.row][self.col])
    end
end

function calc:handle_button(c)
    if self.error or type(self.value) ~= "string" then
        self.value = ""
    end
    self.error = false
    if c == "C" then
        self.value = ""
    elseif c == "=" then
        local status,result = pcall(load,"return "..self.value)
        if status == false or result == nil then
            self.error = true
            self.value = "syntax error"
        else
            status,self.value = pcall(result)
            if self.status == false or self.value == nil then
                self.error = true
                self.value = "syntax error"
            else
                self.value = tostring(self.value)
            end
        end
    else
        self.value = self.value..c
        if #c > 2 then
            self.value = self.value.."("
        end
    end
end

function calc:draw()
    display.draw(function()
        display.rect(self.left-4,self.top-4,self.width+8,self.height+8,self.border,self.border)
        display.rect(self.left,self.top,self.width,self.cell_size,self.border,self.background)
        local fg = self.foreground
        if self.error then fg = self.error_forground end
        display.print(tostring(self.value),self.left + self.pad,self.top + self.pad,self.font,fg,self.background, self.width - self.pad * 2)
        for i=1,self.rows,1 do
            local row = self.buttons[i]
            for j=1,self.cols,1 do
                local c = row[j]
                local x = self.left + (j - 1) * self.cell_size
                local y = self.top + i * self.cell_size
                local bg = self.background
                if i == self.row and j == self.col then bg = self.highlight end
                display.rect(x,y,self.cell_size,self.cell_size,self.border,bg)
                local pad = self.cell_size // 2 - self.font:width(c) // 2
                display.print(c,x + pad, y + self.pad,self.font,self.border,bg)
            end
        end
    end)
end

--export all the math functions to the global namespace
for k,v in pairs(math) do
    if k ~= "type" then
        _G[k] = v
    end
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

function handle_error(error)
    if error == nil then error = "Unknown Error!\n" end
    local f = FONT.MONO_20
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
    local log = logger("CALC.ERR")
    log:write(error)
    log:close()
    keys:anykey()
end

calc:run()

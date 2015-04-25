-- a text editor

function inc(val,min,max)
    if val == max then return min end
    return val + 1
end

function dec(val,min,max)
    if val == min then return max end
    return val - 1
end

filedialog = {}

event.keypress = function(key)
    if editor.running then
        if key ~= 0 and lastkey == 0 then
            lastkey = key
        end
        return false
    end
end

function filedialog:updatefiles()
    local status,items = xpcall(self.current.children,debug.traceback,self.current)
    if status == false then
        print(items)
        display.notify_box(items, 8000)
    end
    if status then self.children = items else self.children = nil end
    status,items = xpcall(self.current.files,debug.traceback,self.current)
    if status == false then
        print(items)
        display.notify_box(items, 8000)
    end
    if status then self.files = items else self.children = nil end
end

function filedialog:open()
    self.current = dryos.directory("ML/SCRIPTS")
    self.selected = 1
    self.scroll = 0
    self:updatefiles()
    self:draw()
    while true do
        local k = lastkey
        lastkey = 0
        if k == KEY.Q then return nil -- cancel
        elseif k == KEY.UP then
            self.selected = self.selected - 1
            if self.selected <= self.scroll then
                self.scroll = self.selected
            end
            self:draw()
        elseif k == KEY.DOWN then 
            self.selected = self.selected + 1
            if self.selected >= self.scroll + (480 - 20 - FONT.LARGE.height) / FONT.MED_LARGE.height - 1  then
                self.scroll = self.scroll + 1
            end
            self:draw()
        elseif k == KEY.SET then
            if self.selected == 0 then
                self.current = dryos.directory(string.gsub(self.current.path,"/.+$",""))
                self.selected = 1
                self.scroll = 0
                self:updatefiles()
                self:draw()
            elseif self.is_dir_selected and self.selected_value ~= nil then
                self.current = self.selected_value
                self.selected = 1
                self.scroll = 0
                self:updatefiles()
                self:draw()
            else
                return self.selected_value
            end
        else
            task.yield(100)
        end
    end
end

function filedialog:draw()
    display.rect(0, 0, 720, 480, COLOR.BLACK, COLOR.BLACK)
    display.print("Please Select A File", 10, 10, FONT.LARGE)
    local pos = 20 + FONT.LARGE.height
    display.line(0, pos, 720, pos, COLOR.WHITE)
    pos = pos + 10
    local dir_count = 0
    local status,items
    self.is_dir_selected = false
    if self.current.exists ~= true then return end
    if self.scroll <= 0 then
        if self.selected == 0 then
            display.print("..", 10, pos, FONT.MED_LARGE, COLOR.WHITE, COLOR.BLUE)
        else
            display.print("..", 10, pos, FONT.MED_LARGE)
        end
        pos = pos + FONT.MED_LARGE.height
    end
    if self.children ~= nil then
        for i,v in ipairs(self.children) do
            if i > self.scroll then
                if i == self.selected then
                    self.is_dir_selected = true
                    self.selected_value = v
                    display.print(v.path, 10, pos, FONT.MED_LARGE, COLOR.WHITE, COLOR.BLUE)
                else
                    display.print(v.path, 10, pos, FONT.MED_LARGE)
                end
                pos = pos + FONT.MED_LARGE.height
                dir_count = i
                if pos > 480 then return end
            end
        end
    end
    if self.files ~= nil then
        for i,v in ipairs(self.files) do
            if dir_count + i > self.scroll then
                if dir_count + i == self.selected then
                    self.selected_value = v
                    display.print(v, 10, pos, FONT.MED_LARGE, COLOR.WHITE, COLOR.BLUE)
                else
                    display.print(v, 10, pos, FONT.MED_LARGE)
                end
                pos = pos + FONT.MED_LARGE.height
                if pos > 480 then return end
            end
        end
    end
end

editor = {}

editor.running = false

editor.menu = menu.new
{
    parent = "Debug",
    name = "Text Editor",
    icon_type = ICON_TYPE.ACTION,
    select = function(this)
        task.create(function() editor:run() end)
    end
}

-- The main program loop
function editor:run()
    local status, error = xpcall(function()
        self.running = true
        menu.block(true)
        self.menu = {"New","Open","Save","Save As","Exit"}
        self.menu_index = 1
        self.menu_open = true
        self.title = "Text Editor"
        self.font = FONT.MONO_20
        self.show_line_numbers = true
        self:draw_title()
        self:draw()
        while true do
            local k = lastkey
            lastkey = 0
            if self.menu_open then
                if self:handle_menu_key(k) == false then return end
            else
                self:handle_key(k)
            end
            task.yield(100)
        end
    end, debug.traceback)
    if status == false then
        print(error)
        display.notify_box(error, 8000)
    end
    menu.block(false)
    self.running = false
end

function editor:handle_key(k)
    if k == KEY.Q then
        self.menu_open = true
        self:draw()
    elseif k == KEY.WHEEL_RIGHT then
        self.scroll = inc(self.scroll,1,#(self.lines))
        self:draw()
    elseif k == KEY.WHEEL_LEFT then
        self.scroll = dec(self.scroll,1,#(self.lines))
        self:draw()
    end
end

function editor:handle_menu_key(k)
    if k == KEY.Q then
        self.menu_open = false
        self:draw()
    elseif k == KEY.DOWN or k == KEY.WHEEL_LEFT then
        self.menu_index = inc(self.menu_index, 1, #(self.menu))
        self:draw()
    elseif k == KEY.UP or k == KEY.WHEEL_RIGHT then
        self.menu_index = dec(self.menu_index, 1, #(self.menu))
        self:draw()
    elseif k == KEY.SET then
        local m = self.menu[self.menu_index]
        if m == "Exit" then return false
        elseif m == "Save" then self:save(self.filename)
        elseif m == "Save As" then self:save()
        elseif m == "New" then self:new()
        elseif m == "Open" then self:open() end
    end
    return true
end

function editor:open()
    local f = filedialog:open()
    if f ~= nil then
        self.filename = f
        self.title = string.format("Text Editor [%s]",f)
        self.lines = {}
        self:draw_status("Loading...")
        for line in io.lines(f) do
            table.insert(self.lines,line)
        end
        self.menu_open = false
        self.line = 1
        self.col = 1
        self.scroll = 1
        self:draw()
    end
end

function editor:new()
    self.filename = nil
    self.title = "Text Editor [untitled*]"
    self.lines = {}
    self.menu_open = false
    self.line = 1
    self.col = 1
    self.scroll = 1
    self:draw()
end

function editor:draw_status(msg)
    local h = FONT.LARGE.height + 40
    local w = FONT.LARGE:width(msg) + 40
    local x = 360 - w / 2
    local y = 240 - h / 2
    display.rect(x,y,w,h,COLOR.WHITE,COLOR.BLACK)
    display.print(msg,x+20,y+20,FONT.LARGE,COLOR.WHITE,COLOR.BLACK)
end

function editor:draw_title()
    display.rect(0, 0, 720, 480, COLOR.BLACK, COLOR.BLACK)
    display.print(self.title, 10, 10, FONT.LARGE)
    local pos = 20 + FONT.LARGE.height
    display.line(0, pos, 720, pos, COLOR.WHITE)
    return pos + 10
end

function editor:draw()
    if self.menu_open then
        local f = FONT.LARGE
        local h = #(self.menu) * f.height + 10
        local w = 180
        local x = 360 - w / 2
        local y = 240 - h / 2
        display.rect(x,y,w,h, COLOR.WHITE, COLOR.BLACK)
        x = x + 5
        y = y + 5
        for i,v in ipairs(self.menu) do
            if i == self.menu_index then
                display.rect(x,y,w-10,f.height,COLOR.BLUE,COLOR.BLUE)
                display.print(v,x,y,f,COLOR.WHITE,COLOR.BLUE)
            else
                display.print(v,x,y,f)
            end
            y = y + f.height
        end
        
    else
        local pos = self:draw_title()
        local pad = 10
        if self.show_line_numbers then
            pad = pad + self.font:width("0000")
            display.line(pad-5,pos,pad-5,480,COLOR.BLUE)
        end
        if self.lines == nil then return end
        for i,v in ipairs(self.lines) do
            if i >= self.scroll then
                if self.show_line_numbers then
                    display.print(string.format("%4d",i),0,pos,self.font,COLOR.BLUE,COLOR.BLACK)
                end
                display.print(v,pad,pos,self.font)
                pos = pos + self.font.height
                if pos > 480 then return end
            end
        end
    end
end

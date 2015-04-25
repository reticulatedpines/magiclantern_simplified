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
editor.first_run = true
editor.min_char = string.byte("A",1)
editor.max_char = string.byte("z",1)

editor.mlmenu = menu.new
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
        display.clear()
        if self.first_run then
            self.menu = {"New","Open","Save","Save As","Exit"}
            self.menu_index = 1
            self.menu_open = true
            self.title = "Text Editor"
            self.font = FONT.MONO_20
            self.show_line_numbers = true
            self:draw_title()
            self.first_run = false
        else
            self.menu_open = false
        end
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
    elseif k == KEY.WHEEL_DOWN then
        self.scroll = inc(self.scroll,1,#(self.lines))
        self:draw()
    elseif k == KEY.WHEEL_UP then
        self.scroll = dec(self.scroll,1,#(self.lines))
        self:draw()
    elseif k ==  KEY.DOWN then
        self.line = inc(self.line,1,#(self.lines))
        self:draw()
    elseif k ==  KEY.UP then
        self.line = dec(self.line,1,#(self.lines))
        self:draw()
    elseif k ==  KEY.RIGHT then
        self.col = inc(self.col,1,#(self.lines[self.line]) + 1)
        self:draw()
    elseif k ==  KEY.LEFT then
        self.col = dec(self.col,1,#(self.lines[self.line]) + 1)
        self:draw()
    elseif k == KEY.WHEEL_LEFT then
        --mod char
        local l = self.lines[self.line]
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = inc(ch,self.min_char,self.max_char)
            self.lines[self.line] = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
            self.lines[self.line] = l..string.char(self.min_char)
        end
        self:draw()
    elseif k == KEY.WHEEL_RIGHT then
        --mod char
        local l = self.lines[self.line]
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = dec(ch,self.min_char,self.max_char)
            self.lines[self.line] = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
            self.lines[self.line] = l..string.char(self.max_char)
        end
        self:draw()
    elseif k == KEY.TRASH then
        --delete
        local l = self.lines[self.line]
        if #l == 0 then
            table.remove(self.lines,self.line)
        elseif self.col > #l and self.line < #(self.lines) then
            self.lines[self.line] = l..self.lines[self.line + 1]
            table.remove(self.lines,self.line + 1)
        else
            self.lines[self.line] = string.format("%s%s",l:sub(1,self.col - 1),l:sub(self.col + 1))
        end
        self:draw()
    elseif k == KEY.SET then
        --insert char
        local l = self.lines[self.line]
        self.lines[self.line] = string.format("%sA%s",l:sub(1,self.col),l:sub(self.col + 1))
        self.col = self.col + 1
        self:draw()
    elseif k == KEY.PLAY then
        --insert line return
        local l = self.lines[self.line]
        self.lines[self.line] = l:sub(1,self.col)
        table.insert(self.lines, self.line + 1, l:sub(self.col + 1))
        self.line = self.line + 1
        self.col = 1
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
    local w = FONT.LARGE:width("Q") + 20
    local h = 20 + FONT.LARGE.height
    local bg = COLOR.gray(5)
    local fg = COLOR.GRAY
    display.rect(0,0,720,h,fg,bg)
    if self.menu_open then
        display.rect(0,0,w,h,fg,COLOR.BLUE)
        display.print("Q",10,10,FONT.LARGE,COLOR.WHITE,COLOR.BLUE)
    else
        display.rect(0,0,w,h,fg,bg)
        display.print("Q",10,10,FONT.LARGE,COLOR.WHITE,bg)
    end
    display.print(self.title,w + 10,10,FONT.LARGE,COLOR.WHITE,bg)
    return h + 10
end

function editor:draw()
    if self.menu_open then
        local bg = COLOR.gray(5)
        local fg = COLOR.GRAY
        local f = FONT.LARGE
        local h = #(self.menu) * f.height + 10
        local w = 180
        local x = 0
        local y = self:draw_title() - 10
        display.rect(x,y,w,h, fg, bg)
        x = x + 5
        y = y + 5
        for i,v in ipairs(self.menu) do
            if i == self.menu_index then
                display.rect(x,y,w-10,f.height,COLOR.BLUE,COLOR.BLUE)
                display.print(v,x,y,f,COLOR.WHITE,COLOR.BLUE)
            else
                display.print(v,x,y,f,COLOR.WHITE,bg)
            end
            y = y + f.height
        end
        
    else
        display.rect(0,0,720,480,COLOR.BLACK,COLOR.BLACK)
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
                if i == self.line then
                    if self.col > #v then
                        local x = pad + self.font:width(v)
                        display.print(" ",x,pos,self.font,COLOR.BLACK,COLOR.WHITE)
                    else
                        local x = pad
                        if self.col > 1 then x = x + self.font:width(string.sub(v,1,self.col - 1)) end
                        local ch = string.sub(v,self.col,self.col)
                        display.print(ch,x,pos,self.font,COLOR.BLACK,COLOR.WHITE)
                    end
                end
                pos = pos + self.font.height
                if pos > 480 then return end
            end
        end
    end
end

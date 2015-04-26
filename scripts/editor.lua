-- a text editor

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
    while self:getkeys() == nil do
        task.yield(100)
    end
    if started then self:stop() end
end

filedialog = {}
filedialog.font = FONT.MED_LARGE
filedialog.top = 60
filedialog.height = 380
filedialog.left = 100
filedialog.width = 520

function filedialog:updatefiles()
    local status,items = xpcall(self.current.children,debug.traceback,self.current)
    if status == false then
        handle_error(items)
    end
    if status then self.children = items else self.children = nil end
    status,items = xpcall(self.current.files,debug.traceback,self.current)
    if status == false then
        handle_error(items)
    end
    if status then self.files = items else self.children = nil end
end

function filedialog:handle_key(k)
    if k == KEY.Q then return "cancel"
    elseif k == KEY.UP or k == KEY.WHEEL_UP or k == KEY.WHEEL_LEFT then
        if self.selected > 0 then
            self.selected = self.selected - 1
            if self.selected <= self.scroll then
                self.scroll = self.selected
            end
        end
    elseif k == KEY.DOWN or k == KEY.WHEEL_DOWN or k == KEY.WHEEL_RIGHT then 
        if self.selected < #(self.files) then
            self.selected = self.selected + 1
            if self.selected >= self.scroll + (self.height - 20 - FONT.LARGE.height) / self.font.height - 1  then
                self.scroll = self.scroll + 1
            end
        end
    elseif k == KEY.SET then
        if self.selected == 0 then
            self.current = dryos.directory(string.gsub(self.current.path,"/.+$",""))
            self.selected = 1
            self.scroll = 0
            self:updatefiles()
        elseif self.is_dir_selected and self.selected_value ~= nil then
            self.current = self.selected_value
            self.selected = 1
            self.scroll = 0
            self:updatefiles()
        else
            return self.selected_value
        end
    end
end

function filedialog:open()
    self.current = dryos.directory("ML/SCRIPTS")
    self.selected = 1
    self.scroll = 0
    self:updatefiles()
    self:draw()
    local started = keyhandler:start()
    while true do
        local keys = keyhandler:getkeys()
        if keys ~= nil then
            for i,v in ipairs(keys) do
                local result = self:handle_key(v)
                if result == "cancel" then 
                    if started then keyhandler:stop() end
                    return nil
                elseif result ~= nil then
                    if started then keyhandler:stop() end
                    return result
                end
            end
            self:draw()
        end
        task.yield(100)
    end
end

function filedialog:draw()
    display.rect(self.left, self.top, self.width, self.height, COLOR.WHITE, COLOR.BLACK)
    display.rect(self.left, self.top, self.width, 20 + FONT.LARGE.height, COLOR.WHITE, COLOR.gray(5))
    display.print("Select File", self.left + 10, self.top + 10, FONT.LARGE,COLOR.WHITE,COLOR.gray(5))
    local pos = self.top + 20 + FONT.LARGE.height
    display.line(self.left, pos, self.width - self.left, pos, COLOR.WHITE)
    local x = self.left + 10
    local r = self.left + self.width
    pos = pos + 10
    local dir_count = 0
    local status,items
    self.is_dir_selected = false
    if self.current.exists ~= true then return end
    if self.scroll <= 0 then
        if self.selected == 0 then
            display.rect(self.left + 1,pos,self.width - 2,self.font.height,COLOR.BLUE,COLOR.BLUE)
            display.print("..", x, pos, self.font, COLOR.WHITE, COLOR.BLUE)
        else
            display.print("..", x, pos, self.font)
        end
        pos = pos + self.font.height
    end
    if self.children ~= nil then
        for i,v in ipairs(self.children) do
            if i > self.scroll then
                if i == self.selected then
                    self.is_dir_selected = true
                    self.selected_value = v
                    display.rect(self.left + 1,pos,self.width - 2,self.font.height,COLOR.BLUE,COLOR.BLUE)
                    display.print(v.path, x, pos, self.font, COLOR.WHITE, COLOR.BLUE)
                else
                    display.print(v.path, x, pos, self.font)
                end
                pos = pos + self.font.height
                dir_count = i
                if (pos + self.font.height) > (self.top + self.height) then return end
            end
        end
    end
    if self.files ~= nil then
        for i,v in ipairs(self.files) do
            if dir_count + i > self.scroll then
                if dir_count + i == self.selected then
                    self.selected_value = v
                    display.rect(self.left + 1,pos,self.width - 2,self.font.height,COLOR.BLUE,COLOR.BLUE)
                    display.print(v, x, pos, self.font, COLOR.WHITE, COLOR.BLUE)
                else
                    display.print(v, x, pos, self.font)
                end
                pos = pos + self.font.height
                if (pos + self.font.height) > (self.top + self.height) then return end
            end
        end
    end
end

editor =
{
    running = false,
    first_run = true,
    min_char = 32,
    max_char = 127,
    show_line_numbers = true,
    menu = 
    {
        {
            name = "File",
            items = {"New","Open","Save","Save As","Exit"},
        },
        {
            name = "Edit",
            items = {"Cut","Copy","Paste","Select All"},
        }
    },
    menu_index = 1,
    submenu_index = 1,
    font = FONT.MONO_20
}

editor.lines_per_page = (460 - FONT.LARGE.height) / editor.font.height - 1

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
            self:new()
            self.first_run = false
        else
            self.menu_open = false
        end
        self:draw()
        keyhandler:start()
        while true do
            local keys = keyhandler:getkeys()
            if keys ~= nil then
                local exit = false
                local redraw = false
                for i,v in ipairs(keys) do
                    if self.menu_open then
                        if self:handle_menu_key(v) == false then
                            exit = true
                            break
                        end
                    else
                        self:handle_key(v)
                    end
                end
                if exit then break end
                self:draw()
            end
            task.yield(100)
        end
    end, debug.traceback)
    if status == false then
        handle_error(error)
    end
    keyhandler:stop()
    menu.block(false)
    self.running = false
end

function editor:handle_key(k)
    if k == KEY.Q then
        self.menu_open = true
    elseif k == KEY.WHEEL_DOWN then
        self.scroll = inc(self.scroll,1,#(self.lines))
    elseif k == KEY.WHEEL_UP then
        self.scroll = dec(self.scroll,1,#(self.lines))
    elseif k ==  KEY.DOWN then
        self.line = inc(self.line,1,#(self.lines))
        self:scroll_into_view()
    elseif k ==  KEY.UP then
        self.line = dec(self.line,1,#(self.lines))
        self:scroll_into_view()
    elseif k ==  KEY.RIGHT then
        self.col = inc(self.col,1,#(self.lines[self.line]) + 1)
        if self.col == 1 then self.line = inc(self.line,1,#(self.lines)) end
        self:scroll_into_view()
    elseif k ==  KEY.LEFT then
        if self.col == 1 then self.line = dec(self.line,1,#(self.lines)) end
        self.col = dec(self.col,1,#(self.lines[self.line]) + 1)
        self:scroll_into_view()
    elseif k == KEY.WHEEL_LEFT then
        --mod char
        self:update_title(true)
        local l = self.lines[self.line]
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = inc(ch,self.min_char,self.max_char)
            self.lines[self.line] = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
            self.lines[self.line] = l..string.char(self.min_char)
        end
        self:scroll_into_view()
    elseif k == KEY.WHEEL_RIGHT then
        --mod char
        self:update_title(true)
        local l = self.lines[self.line]
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = dec(ch,self.min_char,self.max_char)
            self.lines[self.line] = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
            self.lines[self.line] = l..string.char(self.max_char)
        end
        self:scroll_into_view()
    elseif k == KEY.TRASH then
        --delete
        self:update_title(true)
        local l = self.lines[self.line]
        if #l == 0 then
            table.remove(self.lines,self.line)
        elseif self.col > #l and self.line < #(self.lines) then
            self.lines[self.line] = l..self.lines[self.line + 1]
            table.remove(self.lines,self.line + 1)
        else
            self.lines[self.line] = string.format("%s%s",l:sub(1,self.col - 1),l:sub(self.col + 1))
        end
        self:scroll_into_view()
    elseif k == KEY.SET then
        --insert char
        self:update_title(true)
        local l = self.lines[self.line]
        self.lines[self.line] = string.format("%s %s",l:sub(1,self.col),l:sub(self.col + 1))
        self.col = self.col + 1
        self:scroll_into_view()
    elseif k == KEY.PLAY then
        --insert line return
        self:update_title(true)
        local l = self.lines[self.line]
        self.lines[self.line] = l:sub(1,self.col)
        table.insert(self.lines, self.line + 1, l:sub(self.col + 1))
        self.line = self.line + 1
        self.col = 1
        self:scroll_into_view()
    end
end

function editor:scroll_into_view()
    if self.line < self.scroll then self.scroll = self.line
    elseif self.line > (self.scroll + self.lines_per_page) then self.scroll = self.line - self.lines_per_page  + 1 end
end

function editor:update_title(mod, force)
    if self.mod ~= mod or force == true then
        self.mod = mod
        local name = self.filename
        if name == nil then name = "untitled" end
        if mod then
            self.title = string.format("Text Editor [%s*]",name)
        else
            self.title = string.format("Text Editor [%s]",name)
        end
    end
end

function editor:handle_menu_key(k)
    if k == KEY.Q then
        self.menu_open = false
    elseif k == KEY.LEFT or k == KEY.WHEEL_LEFT then
        self.menu_index = dec(self.menu_index, 1, #(self.menu))
        self.submenu_index = 1
    elseif k == KEY.RIGHT or k == KEY.WHEEL_RIGHT then
        self.menu_index = inc(self.menu_index, 1, #(self.menu))
        self.submenu_index = 1
    elseif k == KEY.DOWN or k == KEY.WHEEL_DOWN then
        local m = self.menu[self.menu_index]
        self.submenu_index = inc(self.submenu_index,1,#(m.items))
    elseif k == KEY.UP or k == KEY.WHEEL_UP then
        local m = self.menu[self.menu_index]
        self.submenu_index = dec(self.submenu_index,1,#(m.items))
    elseif k == KEY.SET then
        local m = self.menu[self.menu_index].items[self.submenu_index]
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
        self:update_title(false, true)
        self.lines = {}
        self:draw_status("Loading...")
        for line in io.lines(f) do
            table.insert(self.lines,line)
        end
        self.line = 1
        self.col = 1
        self.scroll = 1
    end
    self.menu_open = false
end

function editor:new()
    self.filename = nil
    self:update_title(true, true)
    self.lines = {""}
    self.menu_open = false
    self.line = 1
    self.col = 1
    self.scroll = 1
end

function editor:save(filename)
    if filename == nil then
        --todo: save file dialog
    else
        self:draw_status("Saving...")
        local f = io.open(filename,"w")
        for i,v in ipairs(self.lines) do
            f:write(v,"\n")
        end
        f:close()
    end
    self:update_title(false, true)
    self.menu_open = false
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

function editor:draw_submenu(m,x,y)
    local bg = COLOR.gray(5)
    local fg = COLOR.GRAY
    local f = FONT.LARGE
    local h = #m * f.height + 10
    local w = 180
    display.rect(x,y,w,h, fg, bg)
    x = x + 5
    y = y + 5
    for i,v in ipairs(m) do
        if i == self.submenu_index then
            display.rect(x,y,w-10,f.height,COLOR.BLUE,COLOR.BLUE)
            display.print(v,x,y,f,COLOR.WHITE,COLOR.BLUE)
        else
            display.print(v,x,y,f,COLOR.WHITE,bg)
        end
        y = y + f.height
    end
end

function editor:draw_menu()
    local bg = COLOR.gray(5)
    local fg = COLOR.GRAY
    local f = FONT.LARGE
    local h = f.height + 10
    local x = 0
    local y = self:draw_title() - 10
    for i,v in ipairs(self.menu) do
        local w = f:width(v.name) + 20
        if i == self.menu_index then
            display.rect(x,y,w,h,fg,COLOR.BLUE)
            display.print(v.name,x + 10,y + 5,f,COLOR.WHITE,COLOR.BLUE)
            self:draw_submenu(v.items,x,y+h)
        else
            display.rect(x,y,w,h,fg,bg)
            display.print(v.name,x + 10,y + 5,f,COLOR.WHITE,bg)
        end
        x = x + w
    end
end

function editor:draw()
    self:draw_main()
    if self.menu_open then
        self:draw_menu()
    end
end

function editor:draw_main()
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
            local clipped = display.print(v,pad,pos,self.font)
            local actual_pos = pos
            local sublines = {}
            if clipped ~= nil then table.insert(sublines,v:sub(1,#v - #clipped)) end
            while clipped ~= nil do
                pos = pos + self.font.height
                local prev = clipped
                clipped = display.print(clipped,pad,pos,self.font)
                if clipped ~= nil then
                    table.insert(sublines,prev:sub(1,#prev - #clipped))
                else
                    table.insert(sublines,prev)
                end
            end
            if i == self.line then
                if self.col > #v then
                    local x = pad + self.font:width(v)
                    if #sublines > 0 then
                        x = pad + self.font:width(sublines[#sublines])
                    end
                    display.print(" ",x,pos,self.font,COLOR.BLACK,COLOR.WHITE)
                else
                    local x = pad
                    local actual_col = self.col
                    local actual_line = v
                    if #sublines > 0 then
                        --figure out what subline we should be on
                        for si,sv in ipairs(sublines) do
                            if actual_col <= #sv then break end
                            actual_pos = actual_pos + self.font.height
                            actual_col = actual_col - #sv
                            actual_line = sv
                        end
                    end
                    if actual_col > 1 then x = x + self.font:width(actual_line:sub(1,actual_col - 1)) end
                    local ch = v:sub(self.col,self.col)
                    display.print(ch,x,actual_pos,self.font,COLOR.BLACK,COLOR.WHITE)
                end
            end
            pos = pos + self.font.height
            if pos > 480 then return end
        end
    end
end

function handle_error(error)
    if error == nil then error = "Unknown Error!\n" end
    local f = FONT.MONO_20
    print(error)
    display.rect(0,0,720,480,COLOR.RED,COLOR.BLACK)
    local pos = 10
    for line in error:gmatch("[^\r\n]+") do
        local clipped = display.print(line,10,pos,f)
        while clipped ~= nil do
            pos = pos + f.height
            clipped = display.print(clipped,10,pos,f)
        end
        pos = pos + f.height
    end
    task.yield(1000)
    keyhandler:anykey()
end
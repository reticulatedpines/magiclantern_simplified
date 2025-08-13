-- Text Editor
-- Edit text files or debug Lua scripts

require("keys")
require("logger")

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

button = {}
button.__index = button

function button.create(caption,x,y,font,w,h)
    local b = {}
    setmetatable(b,button)
    b.font = font
    if b.font == nil then
        b.font = FONT.MED_LARGE
    end
    b.caption = caption
    b.pad = 5
    b.left = x
    b.top = y
    b.width = w
    b.height = h
    b.border = COLOR.WHITE
    b.foreground = COLOR.WHITE
    b.background = COLOR.BLACK
    b.highlight = COLOR.BLUE
    b.disabled_color = COLOR.DARK_GRAY
    b.disabled_background = COLOR.gray(5)
    b.focused = false
    b.disabled = false
    if h == nil then
        b.height = b.font.height + b.pad * 2
    end
    if w == nil then
        b.width = b.font:width(b.caption) + b.pad * 2
    end
    return b
end

function button:draw()
    local bg = self.background
    local fg = self.foreground
    if self.disabled then fg = self.disabled_color end
    if self.focused then
        if self.disabled then bg = self.disabled_background
        else bg = self.highlight end
    end
    display.rect(self.left,self.top,self.width,self.height,self.border,bg)
    local x =  self.width / 2 - self.font:width(self.caption) / 2
    display.print(self.caption,self.left + x,self.top + self.pad,self.font,fg,bg)
end

function button:handle_key(k)
    if k == KEY.SET and self.focused and self.disabled == false then return self.caption end
end

scrollbar = {}
scrollbar.__index = scrollbar

function scrollbar.create(step,min,max,x,y,w,h)
    local sb = {}
    setmetatable(sb,scrollbar)
    sb.step = step
    sb.min = min
    sb.max = max
    sb.value = min
    sb.top = y
    sb.left = x
    sb.width = w
    sb.foreground = COLOR.BLUE
    -- fixme: neither global width nor sb.w appear to be used anywhere
    -- if width == nil then sb.w = 2 end
    sb.height = h
    if h == nil then sb.height = display.height - y end
    return sb
end

function scrollbar:draw()
    --update max automatically from 'table'
    if self.table ~= nil then self.max = #(self.table) end
    --don't draw if we are not needed
    if (self.max - self.min + 1) * self.step <= self.height then return end
    
    local total_height = (self.max - self.min + 1) * self.step
    local thumb_height = self.height * self.height / total_height
    local offset = (self.value - self.min) * self.step * self.height / total_height
    display.rect(self.left,self.top + offset,self.width,thumb_height,self.foreground,self.foreground)
end

function scrollbar:up()
    self.value = dec(self.value,self.min,self.max)
end

function scrollbar:down()
    self.value = inc(self.value,self.min,self.max)
end

textbox = {}
textbox.__index = textbox
function textbox.create(value,x,y,w,font,h)
    local tb = {}
    setmetatable(tb,textbox)
    tb.font = font
    if tb.font == nil then
        tb.font = FONT.MED_LARGE
    end
    tb.min_char = 32
    tb.max_char = 127
    tb.value = value
    tb.pad = 5
    tb.left = x
    tb.top = y
    tb.width = w
    tb.height = h
    tb.border = COLOR.WHITE
    tb.foreground = COLOR.WHITE
    tb.background = COLOR.BLACK
    tb.focused_background = COLOR.BLUE
    tb.col = 1
    if h == nil then
        tb.height = tb.font.height + 10
    end
    return tb
end

function textbox:draw()
    local bg = self.background
    if self.focused then bg = self.focused_background end
    display.rect(self.left,self.top,self.width,self.height,self.border,bg)
    display.print(self.value,self.left + self.pad,self.top + self.pad,self.font,self.foreground,bg)
    local w = self.font:width(self.value:sub(1,self.col - 1))
    if self.col == 1 then w = 0 end
    local ch = self.value:sub(self.col,self.col)
    display.print(ch,self.left + w + self.pad,self.top + self.pad,self.font,self.background,self.foreground)
end

function textbox:handle_key(k)
    if k ==  KEY.RIGHT then
        self.col = inc(self.col,1,#(self.value) + 1)
    elseif k ==  KEY.LEFT then
        self.col = dec(self.col,1,#(self.value) + 1)
    elseif k == KEY.WHEEL_RIGHT then
        --mod char
        local l = self.value
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = inc(ch,self.min_char,self.max_char)
            self.value = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
           self.value = l..string.char(self.min_char)
        end
    elseif k == KEY.WHEEL_LEFT then
        --mod char
        local l = self.value
        if self.col < #l then
            local ch = l:byte(self.col)
            ch = dec(ch,self.min_char,self.max_char)
            self.value = string.format("%s%s%s",l:sub(1,self.col - 1),string.char(ch),l:sub(self.col + 1))
        else
            self.value = l..string.char(self.max_char)
        end
    elseif k == KEY.PLAY then
        --insert
        local l = self.value
        self.value = string.format("%s %s",l:sub(1,self.col),l:sub(self.col + 1))
        self.col = self.col + 1
    elseif k == KEY.TRASH then
        --delete
        local l = self.value
        if self.col <= #l then
            self.value = string.format("%s%s",l:sub(1,self.col - 1),l:sub(self.col + 1))
        end
    end
end

filedialog = {}
filedialog.__index = filedialog
function filedialog.create()
    local fd = {}
    setmetatable(fd,filedialog)
    fd.font = FONT.MED_LARGE
    fd.top = 60
    fd.height = 380
    fd.left = 100
    fd.width = 520
    fd:createcontrols()
    return fd
end

function filedialog:createcontrols()
    self.save_box = textbox.create("",self.left,self.top + self.height - self.font.height - 10,self.width)
    --limit chars to those needed for filenames
    self.save_box.min_char = 46
    self.save_box.max_char = 95
    local w = self.width / 2
    self.ok_button = button.create("OK",self.left,self.top+self.height,self.font,w)
    self.cancel_button = button.create("Cancel",self.left + w,self.top+self.height,self.font,w)
    local title_height = 20 + FONT.LARGE.height
    self.scrollbar = scrollbar.create(self.font.height,0,1,self.left + self.width - 6,self.top + title_height, 2, self.height - title_height - self.save_box.height)
end

function filedialog:updatefiles()
    self.item_count = 0
    local status,items = xpcall(self.current.children,debug.traceback,self.current)
    if status == false then
        handle_error(items)
    end
    if status then 
        self.children = items
        self.item_count = #items
        table.sort(self.children, function(d1,d2) return d1.path < d2.path end)
    else 
        self.children = nil 
    end
    status,items = xpcall(self.current.files,debug.traceback,self.current)
    if status == false then
        handle_error(items)
    end
    if status then
        self.files = items
        self.item_count = self.item_count + #items
        table.sort(self.files)
    else 
        self.files = nil 
    end
    self.scrollbar.max = self.item_count
end

function filedialog:scroll_into_view()
    if self.selected < self.scrollbar.value then
        self.scrollbar.value = self.selected
    elseif self.selected >= self.scrollbar.value + (self.height - 20 - FONT.LARGE.height) / self.font.height - 3  then
        self.scrollbar.value = self.selected - ((self.height - 20 - FONT.LARGE.height) / self.font.height - 3)
    end
end

function filedialog:focus_next()
    if self.save_mode then
        self.focused_index = inc(self.focused_index,1,4)
    else
        self.focused_index = inc(self.focused_index,1,3)
    end
    self:update_focus()
end

function filedialog:update_focus()
    self.focused = (self.focused_index == 1)
    if self.save_mode then
        self.save_box.focused = (self.focused_index == 2)
        self.ok_button.focused = (self.focused_index == 3)
        self.cancel_button.focused = (self.focused_index == 4)
    else
        self.ok_button.focused = (self.focused_index == 2)
        self.cancel_button.focused = (self.focused_index == 3)
    end
end

function filedialog:handle_key(k)
    if k == KEY.MENU or k == key.Q then
        self:focus_next()
    elseif self.save_mode and self.focused_index == 2 then
        return self.save_box:handle_key(k)
    elseif self.focused_index == 2 then
        return self.ok_button:handle_key(k)
    elseif self.save_mode and self.focused_index == 3 then
        return self.ok_button:handle_key(k)
    elseif self.focused_index == 3 then
        return self.cancel_button:handle_key(k)
    elseif self.focused_index == 4 then
        return self.cancel_button:handle_key(k)
    elseif k == KEY.UP or k == KEY.WHEEL_UP then
        self.selected = dec(self.selected,0,self.item_count)
        self:scroll_into_view()
    elseif k == KEY.DOWN or k == KEY.WHEEL_DOWN then 
        self.selected = inc(self.selected,0,self.item_count)
        self:scroll_into_view()
    elseif k == KEY.SET then
        if self.selected == 0 then
            if self.current.parent ~= nil then
                self.current = self.current.parent
                self.selected = 1
                self.scrollbar.value = 0
                self:updatefiles()
            end
        elseif self.is_dir_selected and self.selected_value ~= nil then
            self.current = self.selected_value
            self.selected = 1
            self.scrollbar.value = 0
            self:updatefiles()
        elseif self.save_mode then
            local found = self.current.path:find("/[^/]+$")
            self.save_box.value = self.current.path:sub(found)
        else
            return self.selected_value
        end
    end
end

function filedialog:save(default_name)
    self.save_mode = true
    if default_name ~= nil then
        self.save_box.value = default_name
    end
    if self.save_box.value == nil then
        self.save_box.value = "UNTITLED"
    end
    return self:show()
end

function filedialog:open()
    self.save_mode = false
    return self:show()
end

function filedialog:show()
    if self.current == nil then
        self.current = dryos.directory("ML/SCRIPTS")
        self.selected = 1
        self.scrollbar.value = 0
    end
    self.focused_index = 1
    self:update_focus()
    local w = self.width/2
    self:updatefiles()
    self:draw()
    local started = keys:start()
    while menu.visible do
        local key = keys:getkey()
        if key ~= nil then
            -- process all keys in the queue (until getkey() returns nil), then redraw
            while key ~= nil do
                local result = self:handle_key(key)
                if result == "Cancel" then 
                    if started then keys:stop() end
                    return nil
                elseif result == "OK" then
                    if started then keys:stop() end
                    if self.save_mode then return self.current.path..self.save_box.value
                    else return self.selected_value end
                elseif result ~= nil then
                    if started then keys:stop() end
                    return result
                end
                key = keys:getkey()
            end
            self:draw()
        end
        task.yield(100)
    end
end

function filedialog:draw()
    display.draw(function()
        self:draw_main()
        self.scrollbar:draw()
        if self.save_mode then 
            self.save_box:draw()
        else
            self.ok_button.disabled = self.is_dir_selected
        end
        self.ok_button:draw()
        self.cancel_button:draw()
    end)
end

function filedialog:draw_main()
    display.rect(self.left, self.top, self.width, self.height, COLOR.WHITE, COLOR.BLACK)
    display.rect(self.left, self.top, self.width, 20 + FONT.LARGE.height, COLOR.WHITE, COLOR.gray(10))
    if self.save_mode then
        display.print(string.format("Save | %s",self.current.path), self.left + 10, self.top + 10, FONT.LARGE,COLOR.WHITE,COLOR.gray(10))
    else
        display.print(string.format("Open | %s",self.current.path), self.left + 10, self.top + 10, FONT.LARGE,COLOR.WHITE,COLOR.gray(10))
    end
    local pos = self.top + 20 + FONT.LARGE.height
    display.line(self.left, pos, self.width - self.left, pos, COLOR.WHITE)
    local x = self.left + 10
    local r = self.left + self.width
    pos = pos + 10
    local dir_count = #(self.children)
    local status,items
    local sel_color = COLOR.DARK_GRAY
    if self.focused then sel_color = COLOR.BLUE end
    self.is_dir_selected = false
    if self.current.exists ~= true then return end
    if self.scrollbar.value == 0 then
        if self.selected == 0 then
            display.rect(self.left + 1,pos,self.width - 2,self.font.height,sel_color,sel_color)
            display.print("..", x, pos, self.font, COLOR.WHITE, sel_color)
        else
            display.print("..", x, pos, self.font)
        end
        pos = pos + self.font.height
    end
    if self.children ~= nil then
        for i,v in ipairs(self.children) do
            if i >= self.scrollbar.value then
                if i == self.selected then
                    self.is_dir_selected = true
                    self.selected_value = v
                    display.rect(self.left + 1,pos,self.width - 2,self.font.height,sel_color,sel_color)
                    display.print(v.path, x, pos, self.font, COLOR.WHITE, sel_color)
                else
                    display.print(v.path, x, pos, self.font)
                end
                pos = pos + self.font.height
                if (pos + self.font.height) > (self.top + self.height) then return end
            end
        end
    end
    if self.files ~= nil then
        for i,v in ipairs(self.files) do
            if dir_count + i >= self.scrollbar.value then
                if dir_count + i == self.selected then
                    self.selected_value = v
                    display.rect(self.left + 1,pos,self.width - 2,self.font.height,sel_color,sel_color)
                    display.print(v, x, pos, self.font, COLOR.WHITE, sel_color)
                elseif self.save_mode then
                    display.print(v, x, pos, self.font, COLOR.GRAY, COLOR.BLACK)
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
    max_char = 126,
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
        },
        -- FIXME: doesn't seem to work
        --{
        --    name = "Debug",
        --    items = {"Run","Step Into","Stacktrace","Locals","Detach"},
        --},
        {
            name = "Font",
            items = {}
        }
    },
    filedialog = filedialog.create(),
    menu_index = 1,
    submenu_index = 1,
    font = FONT.MONO_20,
    debugging = false,
    time = 0
}

for k,v in pairs(FONT) do
    table.insert(editor.menu[3].items,k)
end
table.sort(editor.menu[3].items)

editor.lines_per_page = (display.height - 20 - FONT.LARGE.height) / editor.font.height / 2
editor.scrollbar = scrollbar.create(editor.font.height,1,1,display.width - 2,20 + FONT.LARGE.height,2)

-- The main program loop
function editor:run(filename)
    local status, error = xpcall(function()
        self.running = true
        menu.block(true)
        display.clear()
        if filename then
            self:open(filename)
            self.menu_open = false
        elseif self.first_run then
            self:new()
            self.first_run = false
        else
            self.menu_open = false
        end
        self:main_loop()
    end, debug.traceback)
    if status == false then
        debug.sethook()
        self.debugging = false
        handle_error(error)
    end
    keys:stop()
    menu.block(false)
    self.running = false
end

function editor:main_loop()
    menu.block(true)
    self:draw()
    keys:start()
    local exit = false
    while menu.visible and not exit do
        local key = keys:getkey()
        if key ~= nil then
            -- process all keys in the queue (until getkey() returns nil), then redraw
            while key ~= nil do
                if self.menu_open then
                    if self:handle_menu_key(key) == false then
                        exit = true
                        break
                    end
                elseif self.debugging then
                    if self:handle_debug_key(key) == false then
                        exit = true
                        break
                    end
                else
                    self:handle_key(key)
                end
                key = keys:getkey()
            end
            self:draw()
        end
        editor.time = editor.time + 1
        task.yield(100)
    end
    keys:stop()
    if self.running == false then menu.block(false) end
end

function editor:handle_key(k)
    if k == KEY.MENU or k == key.Q then
        self.menu_open = true
    elseif k == KEY.WHEEL_DOWN then
        self.scrollbar:down()
    elseif k == KEY.WHEEL_UP then
        self.scrollbar:up()
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
    elseif k == KEY.WHEEL_RIGHT then
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
    elseif k == KEY.WHEEL_LEFT then
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
        if self.selection_start ~= nil and self.selection_end ~= nil then
            self:delete_selection()
        else
            local l = self.lines[self.line]
            if #l == 0 then
                if #(self.lines) > 1 then
                    table.remove(self.lines,self.line)
                end
            elseif self.col > #l and self.line < #(self.lines) then
                self.lines[self.line] = l..self.lines[self.line + 1]
                table.remove(self.lines,self.line + 1)
            else
                self.lines[self.line] = string.format("%s%s",l:sub(1,self.col - 1),l:sub(self.col + 1))
            end
        end
        self:scroll_into_view()
    elseif k == KEY.LV or k == KEY.REC then
        self:toggle_breakpoint(self.line)
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
    elseif k == KEY.INFO then
        if self.selection_start == nil or self.selection_end ~= nil then
            self.selection_start = {self.line,self.col}
            self.selection_end = nil
        else
            if self.selection_start[1] > self.line or (self.selection_start[1] == self.line and self.selection_start[2] > self.col) then
                self.selection_end = self.selection_start
                self.selection_start = {self.line,self.col}
            else
                self.selection_end = {self.line,self.col}
            end
        end
    end
end

function editor:scroll_into_view()
    if self.line < self.scrollbar.value then self.scrollbar.value = self.line
    elseif self.line > (self.scrollbar.value + self.lines_per_page) then self.scrollbar.value = self.line - self.lines_per_page  + 1 end
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
    if k == KEY.MENU or k == key.Q then
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
        if self:menu_enabled(m) then
            if m == "Exit" then return false
            elseif m == "Save" then self:save(self.filename)
            elseif m == "Save As" then self:save()
            elseif m == "New" then self:new()
            elseif m == "Open" then self:open()
            elseif m == "Cut" then self:copy() self:delete_selection()
            elseif m == "Copy" then self:copy()
            elseif m == "Paste" then self:paste()
            elseif m == "Select All" then
                self.selection_start = {1,1}
                self.selection_end = {#(self.lines),#(self.lines[self.line])}
            elseif m == "Run" then
                if self:save(self.filename) then
                    self:debug()
                end
            elseif m == "Step Into" then
                if self:save(self.filename) then
                    self:debug(true)
                end
            elseif m == "Detach" then
                self.debugging = false
                debug.sethook()
            elseif m == "Stacktrace" then
                self:draw_text(self.stacktrace)
            elseif m == "Locals" then
                self:draw_text(self.locals)
            elseif FONT[m] ~= nil then
                self.font = FONT[m]
                self.scrollbar.step = self.font.height
            end
        end
    end
    return true
end

function editor:menu_enabled(m)
    if self.debugging then
        if m == "Exit" or m == "Copy" or m == "Detach" or m =="Stacktrace" or m == "Locals" or m == "Globals" then
            return true
        else
            return false
        end
    else
        if m == "Detach" or m =="Stacktrace" or m == "Locals" or m == "Globals" then return false
        else return true end
    end
end

function editor:open(f)
    if f == nil then
        f = self.filedialog:open()
    end
    if f ~= nil then
        self.filename = f
        self:update_title(false, true)
        self:draw_status("Loading...")
        local file = io.open(f,"r")
        --this is much faster than io.lines b/c the file io is all done in one large read request
        self.lines = logger.tolines(file:read("*a"))
        file:close()
        self.line = 1
        self.col = 1
        self.scrollbar.table = self.lines
        self.scrollbar.value = 1
        self.breakpoints = {}
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
    self.scrollbar.table = self.lines
    self.scrollbar.value = 1
    self.breakpoints = {}
end

function editor:save(filename)
    if filename == nil then
        local result = self.filedialog:save("UNTITLED.LUA")
        if result ~= nil then
            return self:save(result)
        else
            return false
        end
    else
        self:draw_status("Saving...")
        local f = io.open(filename,"w")
        for i,v in ipairs(self.lines) do
            f:write(v,"\n")
        end
        f:close()
        self.filename = filename
        self:update_title(false, true)
        self.menu_open = false
        return true
    end
end

function editor:copy()
    if self.selection_start ~= nil and self.selection_end ~= nil then
        self.clipboard = {}
        if self.selection_start[1] == self.selection_end[1] then
            --single line selection
            self.clipboard[1] = self.lines[self.selection_start[1]]:sub(self.selection_start[2], self.selection_end[2])
        else
            --multiline selection
            self.clipboard[1] = self.lines[self.selection_start[1]]:sub(self.selection_start[2])
            for i = self.selection_start[1] + 1,self.selection_end[1] - 1,1 do
                table.insert(self.clipboard,self.lines[i])
            end
            table.insert(self.clipboard,self.lines[self.selection_end[1]]:sub(1,self.selection_end[2]))
        end
    else
        editor:message("Error: Nothing Selected!")
    end
    self.menu_open = false
end

function editor:paste()
    if self.clipboard ~= nil and #(self.clipboard) > 0 then
        local c = #(self.clipboard)
        if c == 1 then
            local l = self.lines[self.line]
            self.lines[self.line] = string.format("%s%s%s",l:sub(1,self.col),self.clipboard[1],l:sub(self.col + 1))
        else
            local l = self.lines[self.line]
            self.lines[self.line] = string.format("%s%s",l:sub(1,self.col),self.clipboard[1])
            for i = 2,c - 1,1 do
                table.insert(self.lines,self.line + i - 1,self.clipboard[i])
            end
            table.insert(self.lines,self.line + c - 1, string.format("%s%s",self.clipboard[c],l:sub(self.col + 1)))
        end
        self:update_title(true)
    else
        editor:message("Error: Clipboard Empty!")
    end
    self.menu_open = false
end

function editor.traceback(msg)
    editor.debug_error_msg = msg
    editor.debug_error_info = debug.getinfo(2,"lS")
    editor:capture_locals(3)
    return debug.traceback(msg,2)
end

function editor:debug(step_into)
    if self.filename ~= nil then
        self.debugging = true
        self.debug_error = false
        self.debug_line = -1
        self:draw()
        keys:stop()
        self.step_over = step_into
        self.debug_call = true
        debug.sethook(function(event,line) self:debug_step(event,line) end, "c")
        local status,error = xpcall(dofile, editor.traceback, self.filename)
        keys:start()
        if status == false then
            debug.sethook()
            self.debug_error = true
            self.stacktrace = error
            if self.debug_error_info ~= nil then
                if self.filename == self.debug_error_info.short_src then
                    self.error_line = self.debug_error_info.currentline
                    if type(self.error_line) == "number" then
                        self.line = self.error_line
                        self:scroll_into_view()
                    end
                end
            end
            return false
        end
        return true
    end
    return false
end

function editor:capture_locals(level)
    local name,value
    local i = 1
    self.locals = ""
    while true do
        name,value = debug.getlocal(level,i)
        if name == nil then break end
        if value == nil then
            self.locals = string.format("%s\n%s=(nil)",self.locals,name)
        elseif type(value) == "number" then
            self.locals = string.format("%s\n%s=%d",self.locals,name,value)
        elseif type(value) == "string" then
            self.locals = string.format("%s\n%s='%s'",self.locals,name,value)
        else
            self.locals = string.format("%s\n%s=%s",self.locals,name,type(value))
        end
        i = i + 1
    end
 end
 
function editor:debug_step(event,line)
    local info = debug.getinfo(3,"S")
    if info.short_src == self.filename then
        if self.debug_call then
            --we've entered user code, switch back to line mode so as to catch breakpoints
            debug.sethook()
            debug.sethook(function(event,line) self:debug_step(event,line) end, "l")
            self.debug_call = false
        elseif self.step_over or self.breakpoints[line] then
            self.stacktrace = debug.traceback(nil,3)
            self:capture_locals(4)
            self.line = line
            self.col = 1
            self.debug_line = line
            self:scroll_into_view()
            self:main_loop()
            self.debug_line = -1
            self:draw()
        end
    elseif self.debug_call == false then
        --switch to "c" hook mode, so that our own code runs faster
        --it will never happen that there would be a switch to user code, w/o a function call first
        debug.sethook()
        debug.sethook(function(event,line) self:debug_step(event,line) end, "c")
        self.debug_call = true
    end
end

function editor:toggle_breakpoint(line)
    if self.breakpoints[line] then
        self.breakpoints[line] = false
    else
        self.breakpoints[line] = true
    end
end

function editor:handle_debug_key(k)
    if k == KEY.MENU or k == key.Q then
        self.menu_open = true
    elseif k == KEY.WHEEL_DOWN then
        self.scrollbar:down()
    elseif k == KEY.WHEEL_UP then
        self.scrollbar:up()
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
    elseif k == KEY.SET then
        --step over
        self.step_over = true
        return false
    elseif k == KEY.LV or k == KEY.REC then
        self:toggle_breakpoint(self.line)
    elseif k == KEY.PLAY then
        --continue
        self.step_over = false
        return false
    end
end

function editor:delete_selection()
    if self.selection_start ~= nil and self.selection_end ~= nil then
        if self.selection_start[1] == self.selection_end[1] then
            --single line selection
            self.lines[self.selection_start[1]] = 
                self.lines[self.selection_start[1]]:sub(1,self.selection_start[2] - 1)..
                self.lines[self.selection_start[1]]:sub(self.selection_end[2] + 1)
        else
            --multiline selection
            self.lines[self.selection_start[1]] =  self.lines[self.selection_start[1]]:sub(1,self.selection_start[2] - 1)
            self.lines[self.selection_end[1]] = self.lines[self.selection_end[1]]:sub(self.selection_end[2] + 1)
            for i = self.selection_start[1] + 1,self.selection_end[1] - 1,1 do
                table.remove(self.lines,self.selection_start[1] + 1)
            end
        end
        self:update_title(true)
        self.line = self.selection_start[1]
        self.selection_start = nil
        self.selection_end = nil
    end
end

function editor:message(msg)
    self:draw_status(msg)
    beep()
    keys:anykey()
end

function editor:draw_status(msg)
    local h = FONT.LARGE.height + 40
    local w = FONT.LARGE:width(msg) + 40
    local x = 360 - w / 2
    local y = 240 - h / 2
    display.rect(x,y,w,h,COLOR.WHITE,COLOR.BLACK)
    display.print(msg,x+20,y+20,FONT.LARGE,COLOR.WHITE,COLOR.BLACK)
end

function editor:draw()
    display.draw(function()
        self.drawing = true
        self:draw_main()
        self.scrollbar:draw()
        if self.menu_open then
            self:draw_menu()
        end
        if self.debugging then
            self:draw_debug_error()
        end
        self.drawing = false
    end)
end

function editor:draw_debug_error()
    if self.debug_error and self.debug_error_msg ~= nil then
        display.rect(0,display.height - self.font.height * 2 - 10,display.width,self.font.height*2 + 10,COLOR.RED,COLOR.BLACK)
        local clipped = display.print(self.debug_error_msg,10,display.height - self.font.height*2 - 5,self.font,COLOR.RED,COLOR.BLACK)
        if clipped ~= nil then
            display.print(clipped,10,display.height - self.font.height - 5,self.font,COLOR.RED,COLOR.BLACK)
        end
    end
end

function editor:draw_text(text)
    self.menu_open = false
    local pos = self:draw_title()
    display.rect(0,pos,display.width,display.height-pos,COLOR.BLACK,COLOR.BLACK)
    for line in text:gmatch("[^\r\n]+") do
        local clipped = display.print(line,10,pos,self.font)
        while clipped ~= nil do
            pos = pos + self.font.height
            clipped = display.print(clipped,10,pos,self.font)
        end
        pos = pos + self.font.height
    end
    keys:anykey()
    self.menu_open = true
end

function editor:draw_title()
    local w = FONT.LARGE:width("Q") + 20
    local h = 20 + FONT.LARGE.height
    local bg = COLOR.gray(5)
    local fg = COLOR.GRAY
    if self.debugging then 
        if self.debug_error then
            bg = COLOR.RED
        else
            bg = COLOR.DARK_GREEN1_MOD
        end
    end
    display.rect(0,0,display.width,h,fg,bg)
    if self.menu_open then
        display.rect(0,0,w,h,fg,COLOR.BLUE)
        display.print("M",10,10,FONT.LARGE,COLOR.WHITE,COLOR.BLUE)
    else
        display.rect(0,0,w,h,fg,bg)
        display.print("M",10,10,FONT.LARGE,COLOR.WHITE,bg)
    end
    display.print(self.title,w + 10,10,FONT.LARGE,COLOR.WHITE,bg)
    return h
end

function editor:draw_submenu(m,x,y)
    local bg = COLOR.gray(5)
    local fg = COLOR.GRAY
    local f = FONT.LARGE
    local h = #m * f.height + 10
    local w = 200
    display.rect(x,y,w,h,fg,bg)
    x = x + 5
    y = y + 5
    for i,v in ipairs(m) do
        if self:menu_enabled(v) then
            if i == self.submenu_index then
                display.rect(x,y,w-10,f.height,COLOR.BLUE,COLOR.BLUE)
                display.print(v,x,y,f,COLOR.WHITE,COLOR.BLUE)
            else
                display.print(v,x,y,f,COLOR.WHITE,bg)
            end
        else
            if i == self.submenu_index then
                display.rect(x,y,w-10,f.height,COLOR.DARK_GRAY,COLOR.DARK_GRAY)
                display.print(v,x,y,f,COLOR.GRAY,COLOR.DARK_GRAY)
            else
                display.print(v,x,y,f,COLOR.DARK_GRAY,bg)
            end
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
    local y = self:draw_title()
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

function editor:draw_selection(line_num,line,x,y,sublines)
    if self.selection_start ~= nil and self.selection_end ~= nil then
        if self.selection_start[1] <= line_num and self.selection_end[1] >=line_num then
            local start_offset = 1
            if self.selection_start[1] == line_num then start_offset = self.selection_start[2] end
            local end_offset = #line
            if self.selection_end[1] == line_num then end_offset = self.selection_end[2] end
            if start_offset > 1 then
                x = x + self.font:width(line:sub(1,start_offset - 1))
            end
            local s = line:sub(start_offset,end_offset)
            display.print(s,x,y,self.font,COLOR.WHITE,COLOR.BLUE)
        end
    end
end

function editor:draw_main()
    display.rect(0,0,display.width,display.height,COLOR.BLACK,COLOR.BLACK)
    local pos = self:draw_title()
    pos = pos + 10
    local pad = 10
    local h = self.font.height
    if self.show_line_numbers then
        pad = pad + self.font:width("0000")
        display.line(pad-5,pos,pad-5,display.height,COLOR.BLUE)
    end
    if self.lines == nil then return end
    local scroll = self.scrollbar.value
    for i,v in ipairs(self.lines) do
        if i >= scroll then
            if self.show_line_numbers then
                if self.breakpoints[i] then
                    display.rect(0,pos,pad - 5,h,COLOR.RED,COLOR.RED)
                    display.print(string.format("%4d",i),0,pos,self.font,COLOR.WHITE,COLOR.RED)
                else
                    display.print(string.format("%4d",i),0,pos,self.font,COLOR.BLUE,COLOR.BLACK)
                end
            end
            local bg = COLOR.BLACK
            if self.debugging then
                if i == self.error_line then bg = COLOR.RED
                elseif i == self.debug_line then bg = COLOR.GREEN1 end
            end
            local clipped = display.print(v,pad,pos,self.font,COLOR.WHITE,bg)
            local actual_pos = pos
            local sublines = {}
            if clipped ~= nil then 
                table.insert(sublines,v:sub(1,#v - #clipped))
            end
            while clipped ~= nil do
                pos = pos + h
                local prev = clipped
                clipped = display.print(clipped,pad,pos,self.font,COLOR.WHITE,bg)
                if clipped ~= nil then
                    table.insert(sublines,prev:sub(1,#prev - #clipped))
                else
                    table.insert(sublines,prev)
                end
            end
            self:draw_selection(i,v,pad,pos,sublines)
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
            pos = pos + h
            if pos >= display.height then return end
        end
    end
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
    local log = logger("EDITOR.ERR")
    log:write(error)
    log:close()
    keys:anykey()
end

editor:run(arg[1])

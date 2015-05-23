--copies the shutter, aperture, and iso of the current mode when switching to M mode

--class to keep track of property values using prophandlers
prop_value = {}
prop_value.__index = prop_value

function prop_value.create(...)
    local pv = 
    {
        value = 0,
        previous = 0,
        time = dryos.ms_clock,
        props = {...}
    }
    setmetatable(pv,prop_value)
    return pv
end

function prop_value:enable()
    for i,v in ipairs(self.props) do
        function v.handler(prop,value) self:set(value) end
    end
end

function prop_value:disable()
    for i,v in ipairs(self.props) do
        v.handler = nil
    end
end

function prop_value:set(value)
    if camera.mode ~= MODE.M and value ~= 0 then
        self.previous = self.value
        self.value = value
    end
end

function prop_value:get()
    --ignore value if we've been in the current mode for less than 1 sec
    if dryos.ms_clock - self.time < 1000 then
        return self.previous
    else
        return self.value
    end
end

local shutter_value = prop_value.create(property.SHUTTER,property.SHUTTER_AUTO)
local aperture_value = prop_value.create(property.APERTURE,property.APERTURE_AUTO)
local iso_value = prop_value.create(property.ISO,property.ISO_AUTO)

--will be set as the prop handler for property.SHOOTING_MODE, when enabled
function shooting_mode_handler(self,value)
    if value == MODE.M then
        local s = shutter_value:get()
        if s ~= 0 then camera.shutter.raw = s end
        
        local a = aperture_value:get()
        if a ~= 0 then camera.aperture.raw = a end
        
        local i = iso_value:get()
        if i ~= 0 then camera.iso.raw = i end
    else
        shutter_value.time = dryos.ms_clock
        aperture_value.time = dryos.ms_clock
        iso_value.time = dryos.ms_clock
        --restore ISO to auto when leaving M
        if value == MODE.AV or value == MODE.TV or value == MODE.P then
            camera.iso.raw = 0
        end
    end
end

--[[---------------------------------------------------------------------------
Class for saving/loading config data
use config.data to read/write value(s) you would like to save and restore
@type config
]]
config = {}
config.configs = {}
config.__index = config

--[[---------------------------------------------------------------------------
Create a new config instance, filename will be determined automagically
@param default the default data values
@function create
]]
function config.create(default)
    local cfg = {}
    --determine the config filename automatically based on the script's filename
    local thisfile = debug.getinfo(1,"S").short_src
    assert(thisfile ~= nil, "Could not determine script filename")
    --capture between the last '/' and last '.' in the filename
    local short_name = string.match(thisfile,"/([^/%.]+)%.[^/%.]+$")
    cfg.filename = string.format("%s%s.cfg", dryos.config_dir.path,short_name)
    assert(thisfile ~= cfg.filename, "Could not determine config filename")
    cfg.default = default
    setmetatable(cfg,config)
    cfg.data = cfg:load()
    table.insert(config.configs, cfg)
    if event.config_save == nil then
        event.config_save = function(unused)
            for i,v in ipairs(config.configs) do
                v:saving()
                v:save()
            end
        end
    end
    return cfg
end

--[[---------------------------------------------------------------------------
Create a new config instance from a menu structure, filename will be determined 
automagically
@param m the menu to create a config for
@function create_from_menu
]]
function config.create_from_menu(m)
    local default = {}
    --default values are simply the menu's default values
    default[m.name] = m.value
    if m.submenu ~= nil then
        --todo: recurse into sub-submenus
        for k,v in pairs(m.submenu) do
            default[k] = v.value
        end
    end
    local cfg = config.create(default)
    cfg.menu = m
    --copy back loaded data to the menu structure
    m.value = cfg.data[m.name]
    if m.submenu ~= nil then
        --todo: recurse into sub-submenus
        for k,v in pairs(m.submenu) do
             v.value = cfg.data[m.name]
        end
    end
    return cfg
end

--[[---------------------------------------------------------------------------
Load the config data from file (by executing the file)
@function load
]]
function config:load()
    local status,result = pcall(dofile,self.filename)
    if status and result ~= nil then 
        return result 
    else
        print(result)
        return self.default 
    end
end

--[[---------------------------------------------------------------------------
Called right before config data is saved to file, override this function to
update your config.data when the config is being saved
@function saving
]]
function config:saving()
    --default implementation: save menu structure if there is one
    if self.menu ~= nil then
        self.data[self.menu.name] = self.menu.value
        if self.menu.submenu ~= nil then
            for k,v in pairs(self.menu.submenu) do
                self.data[k] = v.value
            end
        end
    end
end

--[[---------------------------------------------------------------------------
Manually save the config data to file (data is saved automatically when the
ML backend does it's config saving, so calling this function is unecessary
unless you want to do a manual save).
Whatever is in the 'data' field of this instance is saved. Only numbers, 
strings and tables can be saved (no functions, threads or userdata)
@function save
]]
function config:save()
    local f = io.open(self.filename,"w")
    f:write("return ")
    assert(f ~= nil, "Could not save config: "..self.filename)
    config.serialize(f,self.data)
    f:close()
end

--private
function config.serialize(f,o)
    if type(o) == "number" then
        f:write(o)
    elseif type(o) == "string" then
        f:write(string.format("%q", o))
    elseif type(o) == "table" then
        f:write("{\n")
        for k,v in pairs(o) do
            f:write("  [")
            config.serialize(f,k)
            f:write("] = ")
            config.serialize(f,v)
            f:write(",\n")
        end
        f:write("}\n")
    else
        --something we don't know how to serialize, just skip it
    end
end


copy2m_menu = menu.new
{
    parent = "Prefs",
    name = "Copy To M",
    help = "Copy exposure settings when switching to M",
    choices = {"Off","On"},
    value = "Off"
}

function copy2m_menu:select(delta)
    if self.value == "Off" then self.value = "On" else self.value = "Off" end
    copy2m_update(self.value)
end

--start/stop the prop handlers to enable/disable this script's functionality
function copy2m_update(value)
    if value == "On" then
        shutter_value:enable()
        aperture_value:enable()
        iso_value:enable()
        property.SHOOTING_MODE.handler = shooting_mode_handler
    else
        shutter_value:disable()
        aperture_value:disable()
        iso_value:disable()
        property.SHOOTING_MODE.handler = nil
    end
end

config.create_from_menu(copy2m_menu)
copy2m_update(copy2m_menu.value)

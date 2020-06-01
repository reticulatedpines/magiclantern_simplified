--[[---------------------------------------------------------------------------
Functions for saving/loading config data

This module is a lua script (@{config.lua}), you need to explicitly load it
with `require('config')`

@module config
]]
config = {}
config.configs = {}
config.__index = config

local create_internal = function(default,thisfile)
    local cfg = {}
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
Create a new config instance, filename will be determined automagically
@param default the default data values
@function create
]]
function config.create(default)
    --determine the config filename automatically based on the script's filename
    return create_internal(default, debug.getinfo(2,"S").short_src)
end

--[[---------------------------------------------------------------------------
@type config
]]

--[[---------------------------------------------------------------------------
Get/Set the data to store to configuration
@field data
]]

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
    local cfg = create_internal(default,debug.getinfo(2,"S").short_src)
    cfg.menu = m
    --copy back loaded data to the menu structure
    m.value = cfg.data[m.name]
    if m.submenu ~= nil then
        --todo: recurse into sub-submenus
        for k,v in pairs(m.submenu) do
             v.value = cfg.data[k]
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
        f:write(tostring(o))
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

return config

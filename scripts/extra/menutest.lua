-- simple menu test, not complete by any means

mymenu = menu.new
{
    parent = "Debug",
    name = "Lua Menu Test",
    help = "Test of the Lua menu API",
    submenu =
    { 
        {
            name = "Run",
            help = "Run this script.",
        },
        {
            name = "sub-submenu test",
            submenu = 
            {
                {
                    name = "sub-sub item 1",
                    min = 0,
                    max = 1024,
                    unit = UNIT.HEX
                },
                {
                    name = "sub-sub item 2",
                    min = 0,
                    max = 1024,
                    unit = UNIT.HEX
                }
            }
        },
        {
            name = "default value test",
            help = "the default value should be 5",
            min = 0,
            max = 10,
            value = 5
        },
        {
            name = "warning test",
            help = "help for warning test",
            min = 0,
            max = 100,
            warning = function(this) if this.value == 0 then return "this value is not supported" end end,
        },
        {
            name = "info test",
            help = "help for info test",
            min = 0,
            max = 10,
            info = function(this) if this.value == 0 then return "'0' is selected" end end,
            rinfo = function(this) if this.value == 1 then return "*" end end,
        },
        {
            name = "dec test",
            min = 0,
            max = 10000,
            unit = UNIT.DEC,
        },
        {
            name = "hex test",
            min = 0,
            max = 1024,
            unit = UNIT.HEX
        },
        {
            name = "hide test",
            icon_type = ICON_TYPE.ACTION,
            select = function(this) this.hidden = true end,
        },
        {
            name = "choices test",
            choices = { "choice1", "choice2", "choice3" },
        },
        {
            name = "select test",
            help = "values should jump by 2",
            min = 0,
            max = 10,
            select = function(this, delta)
                this.value = this.value + delta * 2
                if this.value < this.min then this.value = this.max
                elseif this.value > this.max then this.value = this.min end
            end
        }
    },
    update = function(this) return this.submenu["choices test"].value end,
}

mymenu.submenu["Run"].select = function(this)
    console.show()
    print("dec test= "..mymenu.submenu["dec test"].value)
    print("choices test= "..mymenu.submenu["choices test"].value)
    print("sub-submenu test= "..mymenu.submenu["sub-submenu test"].submenu["sub-sub item 1"].value)
    print("script run finished!")
end

-- simple menu test, not complete by any means

mymenu = menu.new
{
    parent = "Debug",
    name = "Lua Menu Test",
    help = "Test of the lua menu API",
    submenu =
    { 
        {
            name = "Run",
            help = "Run this script.",
            icon_type = ICON_TYPE.ACTION,
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
            warning = function() return "this is a warning" end,
        },
        {
            name = "info test",
            help = "help for info test",
            min = 0,
            max = 10,
            info = function() return "this is some information" end,
        },
        {
            name = "dec test",
            min = 0,
            max = 10000,
            unit = 7,
        },
        {
            name = "hex test",
            min = 0,
            max = 1024,
            unit = UNIT.HEX
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
            select = function(delta)
                mymenu.submenu["select test"].value = mymenu.submenu["select test"].value + delta * 2
                if mymenu.submenu["select test"].value < mymenu.submenu["select test"].min then mymenu.submenu["select test"].value = mymenu.submenu["select test"].max
                elseif mymenu.submenu["select test"].value > mymenu.submenu["select test"].max then mymenu.submenu["select test"].value = mymenu.submenu["select test"].min end
            end
        }
    },
    update = function() return mymenu.submenu["choices test"].value end,
}

mymenu.submenu["Run"].select = function()
    console.show()
    print("dec test= "..mymenu.submenu["dec test"].value)
    print("choices test= "..mymenu.submenu["choices test"].value)
    print("sub-submenu test= "..mymenu.submenu["sub-submenu test"].submenu["sub-sub item 1"].value)
    print("script run finished!")
end

-- take a screenshot every time a key is pressed

scrnshot_menu = menu.new
{
    parent = "Debug",
    name = "Screenshot on Keypress",
    choices = { "Off", "On" },
    value = "Off",
}

function event.keypress(key)
    if key ~= 0 and scrnshot_menu.value == "On" then
        display.screenshot()
    end
end
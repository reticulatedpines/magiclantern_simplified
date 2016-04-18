-- take a screenshot every time a key is pressed

scrnshot_menu = menu.new
{
    name    = "Screenshot on Keypress",
    choices = { "OFF", "ON" },
    value   = "OFF",
    help    = "Take a screenshot every time a key is pressed",

}

function event.keypress(key)
    if key ~= 0 and scrnshot_menu.value == "ON" then
        display.screenshot()
    end
end

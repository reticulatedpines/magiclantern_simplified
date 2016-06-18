-- Screenshot on Keypress
-- take a screenshot every time a key is pressed.

scrnshot_menu = menu.new
{
    parent  = "Screenshot on Keypress",
    name    = "Enabled",
    choices = { "OFF", "ON" },
    value   = "OFF",
    help    = "Take a screenshot every time a key is pressed",

}

-- fixme: this slows down the GUI a lot
-- also takes many unnecessary screenshots
-- (for example, on press/unpress events)
function event.keypress(key)
    if key ~= 0 and scrnshot_menu.value == "ON" then
        display.screenshot()
    end
end

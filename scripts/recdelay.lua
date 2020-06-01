-- Movie Delayed Start
-- starts movie recording after a delay

recdelay_running = false
recdelay_stop = false
recdelay_countdown = 0

function recdelay_main()
    menu.close()
    if recdelay_running then recdelay_stop = true return end
    if camera.mode == MODE.MOVIE and movie.recording == false and recdelay_menu.submenu["Delay Amount"].value > 0 then
        recdelay_running = true
        recdelay_stop = false
        local i
        for i = recdelay_menu.submenu["Delay Amount"].value, 0, -1 do
            recdelay_countdown = i
            if menu.visible then
                display.notify_box(string.format("Movie Start in %ds", i))
                -- else we'll just let lvinfo item take care of notification
            end
            task.yield(1000)
            if movie.recording or recdelay_stop then
                display.notify_box(string.format("Movie Start Canceled", i))
                recdelay_running = false
                recdelay_stop = false
                return 
            end
        end
        movie.start()
        if recdelay_menu.submenu["Stop After"].value > 0 then
            for i = recdelay_menu.submenu["Stop After"].value, 0, -1 do
                recdelay_countdown = -i
                task.yield(1000)
                if movie.recording == false or recdelay_stop then
                    display.notify_box(string.format("Movie Stop Canceled", i))
                    recdelay_running = false
                    recdelay_stop = false
                    return 
                end
            end
            if movie.recording then movie.stop() end
        end
        recdelay_running = false
    end
end
            
recdelay_menu = menu.new
{
    parent = "Movie",
    name = "Delayed Start",
    help = "Start movie recording after a delay",
    help2 = "Use SET to start or cancel the delay",
    depends_on = DEPENDS_ON.MOVIE_MODE,
    unit = UNIT.TIME,
    submenu = 
    {
        {
            name = "Run",
            help = "Start the delay count down now",
            help2 = "You can also use SET to start or cancel the delay",
            select = function(this) task.create(recdelay_main) end,
            depends_on = DEPENDS_ON.MOVIE_MODE,
        },
        {
            name = "Delay Amount",
            value = 5,
            min = 0,
            max = 600,
            unit = UNIT.TIME,
            update = function(this) recdelay_menu.value = this.value end,
        },
        {
            name = "Stop After",
            value = 10,
            min = 0,
            max = 1800,
            unit = UNIT.TIME
        }
    }
}

event.keypress = function(key)
    if key == KEY.SET and camera.mode == MODE.MOVIE and menu.visible == false then
        if recdelay_running then 
            recdelay_stop = true
        elseif recdelay_menu.submenu["Delay Amount"].value > 0 then
            display.notify_box("Movie Start Trggered")
            task.create(recdelay_main)
        end
    end
end

lv.info
{
    name = "Delayed Start Info",
    value = "",
    priority = 100,
    update = function(this)
        if recdelay_running then
            this.background = COLOR.RED
            this.foreground = COLOR.WHITE
            if recdelay_countdown > 0 then
                this.value = string.format("Start in %ds",recdelay_countdown)
            else
                this.value = string.format("Stop in %ds",-recdelay_countdown)
            end
        else
            this.value = ""
        end
    end
}

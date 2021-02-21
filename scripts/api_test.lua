-- Script API Tests
-- Test routines for the scripting API
-- When adding new Lua APIs, tests for them should go here.

require("logger")

-- global logger
test_log = nil

function printf(s,...)
    test_log:writef(s,...)

    if not console.visible then
        display.notify_box(s:format(...), 5000)
    end
end

function request_mode(mode, mode_str)
    while camera.mode ~= mode do
        printf("Please switch to %s mode.\n", mode_str, mode)
        while camera.mode ~= mode do
            console.show(); assert(console.visible)
            sleep(1)
        end
    end
end

function round(x)
    -- https://scriptinghelpers.org/questions/4850/how-do-i-round-numbers-in-lua-answered
    return x + 0.5 - (x + 0.5) % 1
end

function print_table(t)
    test_log:write(string.format("%s = ", t))
    local s,e = xpcall(function()
        test_log:serialize(_G[t])
    end, debug.traceback)
    if s == false then
        test_log:write(e)
    end
end

-- used for testing Lua 'strict' mode
declared_var = nil

function strict_tests()
    printf("Strict mode tests...\n")

    -- this should work
    declared_var = 5
    assert(declared_var == 5)

    -- this should raise error
    local s,e = pcall(function() undeclared_var = 7 end)
    assert(s == false)
    assert(e:find("assign to undeclared variable 'undeclared_var'"))

    -- same here
    local s,e = pcall(function() print(undeclared_var) end)
    assert(s == false)
    assert(e:find("variable 'undeclared_var' is not declared"))

    printf("Strict mode tests passed.\n")
    printf("\n")
end

function generic_tests()
    printf("Generic tests...\n")
    print_table("arg")
    print_table("camera")
    print_table("event")
    print_table("console")
    print_table("lv")
    print_table("lens")
    print_table("display")
    print_table("key")
    print_table("menu")
    print_table("movie")
    print_table("dryos")
    print_table("interval")
    print_table("battery")
    print_table("task")
    print_table("property")
    printf("Generic tests completed.\n")
    printf("\n")
end

function stdio_test()
    local s,e
    
    -- these should print to console
    -- fixme: they should output this:
    -- Hello, stderr. Hello, stdout. 
    -- Hello again, stdout.
    -- Hello again, stderr.
    io.write("Hello, stdout. ")
    io.stderr:write("Hello, stderr. ")
    io.write("\n")

    io.write("Hello again, stdout.\n")
    io.stderr:write("Hello again, stderr.\n")

    -- not implemented
    -- I expected error, but...
    local inp = io.read("*line")
    print("From stdin:", inp)
end

function copy_test(src, dst)
    printf("Copy test: %s -> %s\n", src, dst)
    
    -- will this copy without errors? we'll see
    -- read source file
    local fin = io.open(src, "rb")
    local data = fin:read("*all")
    fin:close()

    -- save a copy
    local fout = io.open(dst, "w")
    fout:write(data)
    fout:close()

    -- verify
    fin = io.open(dst, "rb")
    local check = fin:read("*all")
    local size = fin:seek("end", 0)
    fin:close()
    assert(data == check)
    assert(size == #data)
    
    -- delete the copy
    assert(dryos.remove(dst) == true)
    
    -- check if it was deleted
    assert(io.open(dst, "rb") == nil)

    -- it should return false this time
    assert(dryos.remove(dst) == false)

    printf("Copy test OK\n")
end

function append_test(file)
    printf("Append test: %s\n", file)

    local data1 = "Allons enfants de la patrie\n"
    local data2 = "Le jour de gloire est arrivé !\n"
    
    -- make sure the file is not there
    dryos.remove(file)
    
    -- create it
    local fout = io.open(file, "a")
    fout:write(data1)
    fout:close()

    -- verify the contents
    local fin = io.open(file, "r")
    local check = fin:read("*all")
    fin:close()
    assert(check == data1)

    -- reopen it to append something
    fout = io.open(file, "a")
    fout:write(data2)
    fout:close()

    -- verify the contents
    fin = io.open(file, "r")
    check = fin:read("*all")
    fin:close()
    assert(check == data1 .. data2)

    -- delete it
    assert(dryos.remove(file) == true)
    
    -- check if it was deleted
    assert(io.open(file, "rb") == nil)

    printf("Append test OK\n")
end

function rename_test(src, dst)
    printf("Rename test: %s -> %s\n", src, dst)

    local data = "Millions saw the apple fall, " ..
        "but Newton was the one who asked why."
    
    -- create the source file
    local fout = io.open(src, "w")
    fout:write(data)
    fout:close()

    -- verify the contents
    local fin = io.open(src, "r")
    local check = fin:read("*all")
    fin:close()
    assert(check == data)

    -- rename it
    assert(dryos.rename(src, dst) == true)

    -- verify the contents
    fin = io.open(dst, "r")
    check = fin:read("*all")
    fin:close()
    assert(check == data)
    
    -- check if the source file was deleted
    assert(io.open(src, "rb") == nil)

    -- cleanup: delete the renamed file
    assert(dryos.remove(dst) == true)
    
    -- check if it was deleted
    assert(io.open(dst, "rb") == nil)

    printf("Rename test OK\n")
end

function test_io()
    printf("Testing file I/O...\n")
    stdio_test()
    copy_test("autoexec.bin", "tmp.bin")
    append_test("tmp.txt")
    rename_test("apple.txt", "banana.txt")
    rename_test("apple.txt", "ML/banana.txt")

    printf("File I/O tests completed.\n")
    printf("\n")
end

function test_camera_gui()
    printf("Testing Canon GUI functions...\n")

    camera.gui.play = true
    assert(camera.gui.play == true)
    assert(camera.gui.mode == 1)

    -- half-shutter should exit playback mode
    key.press(KEY.HALFSHUTTER)
    sleep(1)
    assert(camera.gui.play == false)
    assert(camera.gui.mode == 0)
    key.press(KEY.UNPRESS_HALFSHUTTER)

    -- randomly switch between PLAY, MENU and IDLE (with or without LiveView)
    for i = 1,100 do
        -- we can request MENU or PLAY mode from anywhere
        -- we can only exit these modes if we are already there
        if math.random(1,2) == 1 then
            if math.random(1,2) == 1 then
                printf("Enter PLAY mode...\n");
                camera.gui.play = true
                assert(camera.gui.play == true)
                assert(camera.gui.menu == false)
                assert(camera.gui.idle == false)
                assert(camera.gui.mode == 1)
            elseif camera.gui.play then
                printf("Exit PLAY mode...\n");
                camera.gui.play = false
                assert(camera.gui.play == false)
                assert(camera.gui.menu == false)
                assert(camera.gui.idle == true)
                assert(camera.gui.mode == 0)
            end
        else
            if math.random(1,2) == 1 then
                printf("Enter MENU mode...\n");
                camera.gui.menu = true
                assert(camera.gui.menu == true)
                assert(camera.gui.play == false)
                assert(camera.gui.idle == false)
                assert(camera.gui.mode == 2)
            elseif camera.gui.menu then
                printf("Exit MENU mode...\n");
                camera.gui.menu = false
                assert(camera.gui.menu == false)
                assert(camera.gui.play == false)
                assert(camera.gui.idle == true)
                assert(camera.gui.mode == 0)
            end
        end

        -- also play around with LiveView
        if camera.gui.menu == false and camera.gui.play == false then
            if math.random(1,2) == 1 then
                -- do something with LiveView, but not as often as switching MENU/PLAY
                if not lv.enabled then
                    printf("Start LiveView...\n");
                    lv.start()
                elseif lv.paused then
                    printf("Resume LiveView...\n");
                    lv.resume()
                elseif math.random(1,10) < 9 then
                    -- this gets taken less often than the next one, why?
                    -- fixme: biased random?
                    printf("Pause LiveView...\n");
                    lv.pause()
                else
                    printf("Stop LiveView...\n");
                    lv.stop()
                end
            end
        end
    end

    lv.stop()

    printf("Canon GUI tests completed.\n")
    printf("\n")
end

function test_menu()
    printf("Testing ML menu API...\n")

    menu.open()
    assert(menu.select("Expo", "ISO"))

    -- menu values should be set-able as string (displayed value)
    -- or as integer (internal value)
    assert(menu.set("Expo", "ISO", "200"))
    assert(camera.iso.value == 200)
    sleep(1)

    -- int should be promoted to string because of custom logic in Expo->ISO menu
    assert(menu.set("Expo", "ISO", 1600))
    assert(camera.iso.value == 1600)
    sleep(1)

    -- move selection (not required for set to work; just for user interface)
    assert(menu.select("Expo", "Picture Style"))

    -- picture styles should be set-able as string
    -- numeric works too, as it sets the internal index
    assert(menu.set("Expo", "Picture Style", "Portrait"))
    assert(menu.get("Expo", "Picture Style") == "Portrait")
    sleep(1)
    assert(menu.set("Expo", "Picture Style", 5)) -- OK, selects Neutral
    -- skip string test, as other camera models may use different indices
    sleep(1)
    assert(menu.set("Expo", "Picture Style", "Landscape"))
    assert(menu.get("Expo", "Picture Style") == "Landscape")
    sleep(1)
    assert(menu.set("Expo", "Picture Style", 1234) == false) -- should fail, out of range
    assert(menu.get("Expo", "Picture Style") == "Landscape") -- old selection should not change
    sleep(1)

    assert(menu.select("Overlay"))
    assert(menu.select("Movie"))
    assert(menu.select("Shoot"))

    assert(menu.select("Shoot", "Advanced Bracket"))

    -- boolean items should be set-able as int (0 or 1)
    assert(menu.set("Shoot", "Advanced Bracket", 1))
    assert(menu.get("Shoot", "Advanced Bracket", 0) == 1)
    sleep(1)

    -- or as string (if the user interface displays "ON" or "OFF")
    -- here, actual string will be "ON, 10s" or similar (also accepted)
    assert(menu.set("Shoot", "Intervalometer", "ON"))
    assert(menu.get("Shoot", "Intervalometer", 0) == 1)
    sleep(1)

    -- turning off should be straightforward
    assert(menu.set("Shoot", "Advanced Bracket", "OFF"))
    assert(menu.get("Shoot", "Advanced Bracket", 0) == 0)
    assert(menu.get("Shoot", "Advanced Bracket") == "OFF")
    sleep(1)

    assert(menu.set("Shoot", "Intervalometer", 0))
    assert(menu.get("Shoot", "Intervalometer", 0) == 0)
    assert(menu.get("Shoot", "Intervalometer") == "OFF")
    sleep(1)

    -- move to Intervalometer menu
    assert(menu.select("Shoot", "Intervalometer"))
    sleep(1)

    -- enter the submenu
    assert(menu.select("Intervalometer", "Take a pic every"))
    sleep(1)

    -- note: setting menu by string works by brute force
    -- that is, trying every possible value and comparing the string
    -- the range for this menu is huge, so it only checks round values
    -- for speed reasons (so entering 1m10s will fail)
    -- smaller ranges are OK for trying every single value
    assert(menu.set("Intervalometer", "Take a pic every", "1m30s"))
    assert(menu.get("Intervalometer", "Take a pic every", 0) == 90)
    assert(menu.get("Intervalometer", "Take a pic every") == "1m30s")
    sleep(1)

    -- actual string will be 10s
    assert(menu.set("Intervalometer", "Take a pic every", "10"))
    assert(menu.get("Intervalometer", "Take a pic every", 0) == 10)
    assert(menu.get("Intervalometer", "Take a pic every") == "10s")
    sleep(1)

    -- integer should work as well - e.g. 1m10s should work now
    assert(menu.set("Intervalometer", "Take a pic every", 70))
    assert(menu.get("Intervalometer", "Take a pic every", 0) == 70)
    assert(menu.get("Intervalometer", "Take a pic every") == "1m10s")
    sleep(1)

    -- out of range, should fail
    assert(menu.set("Intervalometer", "Take a pic every", 7000000) == false)
    assert(menu.get("Intervalometer", "Take a pic every", 0) == 70)
    assert(menu.get("Intervalometer", "Take a pic every") == "1m10s")
    sleep(1)

    -- exit submenu
    assert(menu.select("Shoot", "Intervalometer"))
    sleep(1)

    -- let's try some more submenus
    -- todo: more multi-level submenus
    assert(menu.select("Advanced Bracket", "Frames")); sleep(1)
    assert(menu.select("Advanced Bracket", "Sequence")); sleep(1)
    assert(menu.select("Advanced Bracket", "ISO shifting")); sleep(1)
    assert(menu.select("Bulb Timer", "Exposure duration")); sleep(1)
    assert(menu.select("Shoot Preferences", "Snap Simulation")); sleep(1)
    assert(menu.select("Misc key settings", "Sticky HalfShutter")); sleep(1)
    assert(menu.select("Play mode actions", "Trigger key(s)")); sleep(1)
    assert(menu.select("LiveView zoom tweaks", "Zoom on HalfShutter")); sleep(1)
    assert(menu.select("Lens info", "Lens ID")); sleep(1)
    assert(menu.select("Shoot", "Intervalometer")); sleep(1)

    -- selection in the Modified menu
    -- note: we have already modified "Take a pic every"
    -- so at least this setting should appear there
    assert(menu.set("Shoot", "Intervalometer", "ON")); sleep(1)
    assert(menu.select("Modified", "Intervalometer")); sleep(1)             -- we should find Intervalometer in the Modified menu
    assert(menu.select("Modified", "Take a pic every")); sleep(1)           -- same for "Take pic every" (smaller font)
    assert(menu.select("Modified", "Intervalometer")); sleep(1)             -- scroll back to intervalometer
    assert(menu.set("Shoot", "Intervalometer", "OFF")); sleep(1)            -- turn it off
    assert(menu.select("Modified", "Take a pic every") == false); sleep(1)  -- now "Take a pic every" should disappear (submenus are only expanded when the main entry is on)
    assert(menu.set("Shoot", "Intervalometer", "ON")); sleep(1)             -- re-enable intervalometer
    assert(menu.select("Modified", "Take a pic every")); sleep(1)           -- now "Take a pic every" should re-appear
    assert(menu.set("Intervalometer", "Take a pic every", 10)); sleep(1)    -- set "Take a pic every" back to default
    assert(menu.select("Modified", "Intervalometer")); sleep(1)             -- scroll back to intervalometer
    assert(menu.set("Shoot", "Intervalometer", "OFF")); sleep(1)            -- turn it off
    assert(menu.select("Modified", "Take a pic every") == false); sleep(1)  -- now "Take a pic every" should disappear
    assert(menu.select("Shoot", "Intervalometer")); sleep(1)                -- move back to Shoot -> Intervalometer
    assert(menu.select("Intervalometer", "Take a pic every")); sleep(1)     -- set other intervalometer settings to default (just in case)
    assert(menu.select("Intervalometer", "Start trigger")); sleep(1)        -- the menu.select calls are just for visual effect
    assert(menu.set("Intervalometer", "Start trigger", "Leave Menu")); sleep(0.5);
    assert(menu.select("Intervalometer", "Start after")); sleep(0.5)
    assert(menu.set("Intervalometer", "Start after", "3s")); sleep(0.5);
    assert(menu.select("Intervalometer", "Stop after")); sleep(0.5)
    assert(menu.set("Intervalometer", "Stop after", "Disabled")); sleep(0.5);
    assert(menu.select("Shoot", "Intervalometer")); sleep(1)
    assert(menu.select("Modified", "Intervalometer") == false); sleep(1)    -- now Modified -> Intervalometer should disappear

    -- non-existent menus; should fail
    assert(menu.select("Dinosaur") == false)
    assert(menu.select("Shoot", "Crocodile") == false)

    -- menu.get/set return nil if the menu was not found
    assert(menu.get("Shoot", "Introvolometer", 0) == nil)
    assert(menu.get("Shoot", "Brack") == nil)
    assert(menu.set("Shoot", "Introvolometer", 1) == nil)
    assert(menu.set("Shoot", "Introvolometer", "OFF") == nil)

    menu.close()

    -- exercise the menu backend a bit
    for i = 1,5 do
        menu.open()
        assert(camera.gui.idle == false)
        menu.close()
        assert(camera.gui.idle == true)
    end

    printf("Menu tests completed.\n")
    printf("\n")
end

function taskA()
    printf("Task A started.\n")
    local i
    for i = 1,100 do
        printf("Task A: %d\n", i)
        task.yield(math.random(10,50))
    end
end

function taskB()
    printf("Task B started.\n")
    local i
    for i = 1,100 do
        printf("Task B: %d\n", i)
        task.yield(math.random(10,50))
    end
end

function taskC()
    printf("Task C started.\n")
    msleep(math.random(10,50))
    printf("Task C finished.\n")
end

function taskD()
    io.write("Task D started.\n")
    msleep(math.random(10,50))
    io.write("Task D finished.\n")
end

function test_multitasking()
    printf("Testing multitasking...\n")

    -- FIXME: this fails
    --task.create(taskA)
    --task.create(taskB)
    --task.yield(5000)
    
    printf("Only one task allowed to interrupt...\n")

    -- small test with logging to file
    for i = 1,10 do
        task.create(taskC)
        printf("Main task yielding.\n")
        task.yield(math.random(10,50))
        printf("Main task back.\n")
    end

    -- larger test with logging to console only
    for i = 1,1000 do
        task.create(taskD)
        io.write("Main task yielding.\n")
        task.yield(math.random(10,50))
        io.write("Main task back.\n")
    end

    task.yield(500)

    printf("Multitasking tests completed.\n")
    printf("\n")
end

function test_keys()
    printf("Testing half-shutter...\n")
    for i = 1,10 do
        -- open Canon menu
        camera.gui.menu = true
        sleep(1)
        assert(camera.gui.menu == true)
        assert(camera.gui.idle == false)
        key.press(KEY.HALFSHUTTER)
        sleep(0.2)
        assert(key.last == KEY.HALFSHUTTER)
        sleep(1)
        -- half-shutter should close Canon menu
        assert(camera.gui.menu == false)
        assert(camera.gui.idle == true)
        key.press(KEY.UNPRESS_HALFSHUTTER)
        sleep(0.2)
        assert(key.last == KEY.UNPRESS_HALFSHUTTER)
    end
    printf("Half-shutter test OK.\n")
    
    -- todo: test other key codes? press/unpress events?
    printf("\n")
end

function test_camera_exposure()
    printf("Testing exposure settings...\n")
    printf("Camera    : %s (%s) %s\n", camera.model, camera.model_short, camera.firmware)
    printf("Lens      : %s\n", lens.name)
    printf("Shoot mode: %s\n", camera.mode)
    printf("Shutter   : %s (raw %s, %ss, %sms, apex %s)\n", camera.shutter, camera.shutter.raw, camera.shutter.value, camera.shutter.ms, camera.shutter.apex)
    printf("Aperture  : %s (raw %s, f/%s, apex %s)\n", camera.aperture, camera.aperture.raw, camera.aperture.value, camera.aperture.apex)
    printf("Av range  : %s..%s (raw %s..%s, f/%s..f/%s, apex %s..%s)\n",
        camera.aperture.min, camera.aperture.max,
        camera.aperture.min.raw, camera.aperture.max.raw,
        camera.aperture.min.value, camera.aperture.max.value,
        camera.aperture.min.apex, camera.aperture.max.apex
    )
    printf("ISO       : %s (raw %s, %s, apex %s)\n", camera.iso, camera.iso.raw, camera.iso.value, camera.iso.apex)
    printf("EC        : %s (raw %s, %s EV)\n", camera.ec, camera.ec.raw, camera.ec.value)
    printf("Flash EC  : %s (raw %s, %s EV)\n", camera.flash_ec, camera.flash_ec.raw, camera.flash_ec.value)

    request_mode(MODE.M, "M")
    local old_value = camera.shutter.raw
    printf("Setting shutter to random values...\n")
    for k = 1,100 do
        local method = math.random(1,4)
        local d = nil
        if method == 1 then
            local s = math.random(1,30)
            if math.random(1,2) == 1 then
                camera.shutter.value = s
            else
                camera.shutter = s
            end
            d = math.abs(math.log(camera.shutter.value,2) - math.log(s,2))
        elseif method == 2 then
            local ms = math.random(1,30000)
            camera.shutter.ms = ms
            d = math.abs(math.log(camera.shutter.ms,2) - math.log(ms,2))
        elseif method == 3 then
            local apex = math.random(-5*100,12*100)/100
            camera.shutter.apex = apex
            d = math.abs(camera.shutter.apex - apex)
        elseif method == 4 then
            local raw = math.random(16,152)
            camera.shutter.raw = raw
            d = math.abs(camera.shutter.raw - raw) / 8
        end

        -- difference between requested and actual shutter speed should be max 1.5/8 EV
        -- (most cameras accept 0, 3/8, 4/8, 5/8, 1 EV, the largest gap being 3/8 EV,
        --  and you should always get the nearest valid shutter speed)
        if d > 1.5/8 + 1e-3 then
            printf("Error: shutter delta %s EV\n", d)
        end

        -- seconds and ms fields should be consistent
        if math.abs(camera.shutter.value - camera.shutter.ms/1000) > 1e-3 then
            printf("Error: shutter %ss != %sms\n", camera.shutter.value, camera.shutter.ms)
        end
        
        -- seconds and apex should be consistent
        -- see http://dougkerr.net/Pumpkin/articles/APEX.pdf
        local expected_apex = math.log(1/camera.shutter.value,2)
        if math.abs(expected_apex - camera.shutter.apex) > 0.01 then
            printf("Error: shutter %ss != Tv%s, expected %s\n", camera.shutter.value, camera.shutter.apex, expected_apex)
        end
        
        -- setting shutter to the same value, using any method (s,ms,apex,raw)
        -- should not change anything
        for i,field in pairs{"value","ms","apex","raw"} do
            local current = {}
            current.apex  = camera.shutter.apex
            current.value = camera.shutter.value
            current.ms    = camera.shutter.ms
            current.raw   = camera.shutter.raw
            
            -- ms is integer, so tests at very low values will fail because of roundoff errors
            -- however, if we have set the shutter in ms from the beginning, it will work fine
            if field == "ms" and current.ms < 10 and method ~= 2 then
                -- how do you write "continue" in Lua?!
                field = "value"
            end
            
            camera.shutter[field] = current[field]
            
            if camera.shutter.value ~= current.value then
                printf("Error: shutter set to %s=%s, got %ss, expected %ss\n", field, current[field], camera.shutter.value, current.value)
            end
            if camera.shutter.ms ~= current.ms then
                printf("Error: shutter set to %s=%s, got %sms, expected %sms\n", field, current[field], camera.shutter.ms, current.ms)
            end
            if camera.shutter.apex ~= current.apex then
                printf("Error: shutter set to %s=%s, got Tv%s, expected Tv%s\n", field, current[field], camera.shutter.apex, current.apex)
            end
            if camera.shutter.raw ~= current.raw then
                printf("Error: shutter set to %s=%s, got %s, expected %s (raw)\n", field, current[field], camera.shutter.raw, current.raw)
            end
        end
    end
    camera.shutter.raw = old_value

    request_mode(MODE.M, "M")
    old_value = camera.iso.raw
    printf("Setting ISO to random values...\n")
    for k = 1,100 do
        local method = math.random(1,3)
        local d = nil
        if method == 1 then
            local iso = math.random(100, 6400)
            if math.random(1,2) == 1 then
                camera.iso.value = iso
            else
                camera.iso = iso
            end
            d = math.abs(math.log(camera.iso.value,2) - math.log(iso,2))
        elseif method == 2 then
            local apex = math.random(5*100,11*100)/100
            camera.iso.apex = apex
            d = math.abs(camera.iso.apex - apex)
        elseif method == 3 then
            local raw = math.random(72, 120)
            camera.iso.raw = raw
            d = math.abs(camera.iso.raw - raw) / 8
        end

        -- difference between requested and actual ISO should be max 1/3 EV
        if d > 1/3 then
            printf("Error: ISO delta %s EV\n", d)
        end

        -- ISO and Sv (APEX) should be consistent
        local expected_apex = math.log(camera.iso.value/3.125, 2)
        if math.abs(expected_apex - camera.iso.apex) > 0.2 then
            printf("Error: ISO %s != Sv%s, expected %s\n", camera.iso.value, camera.iso.apex, expected_apex)
        end

        -- setting ISO to the same value, using any method (value,apex,raw)
        -- should not change anything
        for i,field in pairs{"value","apex","raw"} do
            local current = {}
            current.apex  = camera.iso.apex
            current.value = camera.iso.value
            current.raw   = camera.iso.raw
            
            camera.iso[field] = current[field]
            
            if camera.iso.value ~= current.value then
                printf("Error: ISO set to %s=%s, got %s, expected %s\n", field, current[field], camera.iso.value, current.value)
            end
            if camera.iso.apex ~= current.apex then
                printf("Error: ISO set to %s=%s, got Sv%s, expected Sv%s\n", field, current[field], camera.iso.apex, current.apex)
            end
            if camera.iso.raw ~= current.raw then
                printf("Error: ISO set to %s=%s, got %s, expected %s (raw)\n", field, current[field], camera.iso.raw, current.raw)
            end
        end
    end
    camera.iso.raw = old_value

    if camera.aperture.min.raw == camera.aperture.max.raw then
        printf("This lens does not have variable aperture (skipping test).\n")
    else
        request_mode(MODE.M, "M")
        old_value = camera.aperture.raw
        printf("Setting aperture to random values...\n")
        for k = 1,100 do
            local method = math.random(1,3)
            local d = nil
            local extra_tol = 0
            if method == 1 then
                local av = math.random(round(camera.aperture.min.value*10), round(camera.aperture.max.value*10)) / 10
                if math.random(1,2) == 1 then
                    camera.aperture.value = av
                else
                    camera.aperture = av
                end
                d = math.abs(math.log(camera.aperture.value,2) - math.log(av,2)) * 2
                -- when checking the result, allow a larger difference (0.1 units) - see note below
                extra_tol = math.abs(math.log(av,2) - math.log(av-0.1,2)) * 2
            elseif method == 2 then
                local apex = math.random(round(camera.aperture.min.apex*100), round(camera.aperture.max.apex*100)) / 100
                camera.aperture.apex = apex
                d = math.abs(camera.aperture.apex - apex)
            elseif method == 3 then
                local raw = math.random(camera.aperture.min.raw, camera.aperture.max.raw)
                camera.aperture.raw = raw
                d = math.abs(camera.aperture.raw - raw) / 8
            end

            -- difference between requested and actual aperture should be max 1.5/8 EV
            -- note: when using F-numbers, the difference may be larger, because of the rounding done
            -- to match Canon values (e.g. raw 48 would be f/5.66 (f/5.7), instead of Canon's f/5.6)
            if (d > 1.5/8 + extra_tol) then
                printf("Error: aperture delta %s EV (expected < %s, %s, method=%d)\n", d, 1.5/8 + extra_tol, camera.aperture, method)
            end

            -- aperture and Av (APEX) should be consistent
            local expected_apex = math.log(camera.aperture.value, 2) * 2
            if math.abs(expected_apex - camera.aperture.apex) > 0.2 then
                printf("Error: aperture %s != Av%s, expected %s\n", camera.aperture.value, camera.aperture.apex, expected_apex)
            end

            -- setting aperture to the same value, using any method (value,apex,raw)
            -- should not change anything
            for i,field in pairs{"value","apex","raw"} do
                local current = {}
                current.apex  = camera.aperture.apex
                current.value = camera.aperture.value
                current.raw   = camera.aperture.raw
                
                camera.aperture[field] = current[field]
                
                if camera.aperture.value ~= current.value then
                    printf("Error: aperture set to %s=%s, got %s, expected %s\n", field, current[field], camera.aperture.value, current.value)
                end
                if camera.aperture.apex ~= current.apex then
                    printf("Error: aperture set to %s=%s, got Sv%s, expected Sv%s\n", field, current[field], camera.aperture.apex, current.apex)
                end
                if camera.aperture.raw ~= current.raw then
                    printf("Error: aperture set to %s=%s, got %s, expected %s (raw)\n", field, current[field], camera.aperture.raw, current.raw)
                end
            end
        end
        camera.aperture.raw = old_value
    end

    request_mode(MODE.AV, "Av")
    old_value = camera.ec.raw
    printf("Setting EC to random values...\n")
    for k = 1,100 do
        local method = math.random(1,2)
        local d = nil
        if method == 1 then
            local ec = math.random(-2*100, 2*100) / 100
            if math.random(1,2) == 1 then
                camera.ec.value = ec
            else
                camera.ec = ec
            end
            d = math.abs(camera.ec.value - ec,2)
        elseif method == 2 then
            local raw = math.random(-16, 16)
            camera.ec.raw = raw
            d = math.abs(camera.ec.raw - raw) / 8
        end

        -- difference between requested and actual EC should be max 1.5/8 EV
        if d > 1.5/8 then
            printf("Error: EC delta %s EV\n", d)
        end

        -- EC and raw should be consistent
        local expected_ec = camera.ec.raw / 8
        if math.abs(expected_ec - camera.ec.value) > 0.001 then
            printf("Error: EC raw %s != %s EV, expected %s EV\n", camera.ec.raw, camera.ec.value, expected_ec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            local current = {}
            current.value = camera.ec.value
            current.raw   = camera.ec.raw
            
            camera.ec[field] = current[field]
            
            if camera.ec.value ~= current.value then
                printf("Error: EC set to %s=%s, got %s, expected %s EV\n", field, current[field], camera.ec.value, current.value)
            end
            if camera.ec.raw ~= current.raw then
                printf("Error: EC set to %s=%s, got %s, expected %s (raw)\n", field, current[field], camera.ec.raw, current.raw)
            end
        end
    end
    camera.ec.raw = old_value

    -- copy/paste & replace from EC (those two should behave in the same way)
    old_value = camera.flash_ec.raw
    printf("Setting Flash EC to random values...\n")
    for k = 1,100 do
        local method = math.random(1,2)
        local d = nil
        if method == 1 then
            local fec = math.random(-2*100, 2*100) / 100
            if math.random(1,2) == 1 then
                camera.flash_ec.value = fec
            else
                camera.flash_ec = fec
            end
            d = math.abs(camera.flash_ec.value - fec,2)
        elseif method == 2 then
            local raw = math.random(-16, 16)
            camera.flash_ec.raw = raw
            d = math.abs(camera.flash_ec.raw - raw) / 8
        end

        -- difference between requested and actual EC should be max 1.5/8 EV
        if d > 1.5/8 then
            printf("Error: FEC delta %s EV\n", d)
        end

        -- EC and raw should be consistent
        local expected_fec = camera.flash_ec.raw / 8
        if math.abs(expected_fec - camera.flash_ec.value) > 0.001 then
            printf("Error: FEC raw %s != %s EV, expected %s EV\n", camera.flash_ec.raw, camera.flash_ec.value, current.expected_fec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            local current = {}
            current.value = camera.flash_ec.value
            current.raw   = camera.flash_ec.raw
            
            camera.flash_ec[field] = current[field]
            
            if camera.flash_ec.value ~= current.value then
                printf("Error: FEC set to %s=%s, got %s, expected %s EV\n", field, current[field], camera.flash_ec.value, current.value)
            end
            if camera.flash_ec.raw ~= current.raw then
                printf("Error: FEC set to %s=%s, got %s, expected %s (raw)\n", field, current[field], camera.flash_ec.raw, current.raw)
            end
        end
    end
    camera.flash_ec.raw = old_value

    printf("Exposure tests completed.\n")
    printf("\n")
end

function print_file_size(filename)
    local f = io.open(filename, "rb")
    if f then
        local size = f:seek("end", 0)
        f:close()
        printf("%s: %s\n", filename, size);
        return size
    else
        printf("%s not found.\n", filename);
    end
end

function test_camera_take_pics()
    printf("Testing picture taking functions...\n")
    local initial_file_num

    request_mode(MODE.M, "M")
    camera.shutter = 1/50
    sleep(2)
    
    printf("Snap simulation test...\n")
    assert(menu.set("Shoot Preferences", "Snap Simulation", 1))
    initial_file_num = dryos.shooting_card.file_number
    camera.shoot()
    assert(dryos.shooting_card.file_number == initial_file_num)
    assert(menu.set("Shoot Preferences", "Snap Simulation", 0))

    sleep(2)

    printf("Single picture...\n")
    -- let's also check if we can find the image file
    -- fixme: a way to check these routines when the image number wraps around at 10000
    initial_file_num = dryos.shooting_card.file_number
    local image_path = dryos.dcim_dir.path ..  dryos.image_prefix .. string.format("%04d", (initial_file_num + 1) % 10000)
    local image_path_cr2 = image_path .. ".CR2"
    local image_path_jpg = image_path .. ".JPG"
    -- the image file(s) should not be present before taking the picture :)
    assert(io.open(image_path_cr2, "rb") == nil)
    assert(io.open(image_path_jpg, "rb") == nil)

    camera.shoot()

    -- but either CR2 or JPG should be there afterwards (or maybe both)
    assert((dryos.shooting_card.file_number - initial_file_num) % 10000 == 1)
    camera.wait()
    local size_cr2 = print_file_size(image_path_cr2)
    local size_jpg = print_file_size(image_path_jpg)
    assert(size_cr2 or size_jpg)

    -- let's review it
    assert(camera.gui.qr or camera.gui.idle)
    camera.gui.play = true
    assert(camera.gui.play == true)
    assert(camera.gui.play_photo == true)
    assert(camera.gui.play_movie == false)

    sleep(2)

    printf("Two burst pictures...\n")
    printf("Ideally, the camera should be in some continuous shooting mode (not checked).\n")
    initial_file_num = dryos.shooting_card.file_number

    -- we should also be able to change the file prefix
    local old_prefix = dryos.image_prefix
    dryos.image_prefix = "ABC_"
    assert(dryos.image_prefix == "ABC_")
    local image1_path = dryos.dcim_dir.path ..  dryos.image_prefix .. string.format("%04d", (initial_file_num + 1) % 10000)
    local image1_path_cr2 = image1_path .. ".CR2"
    local image1_path_jpg = image1_path .. ".JPG"
    local image2_path = dryos.dcim_dir.path ..  dryos.image_prefix .. string.format("%04d", (initial_file_num + 2) % 10000)
    local image2_path_cr2 = image2_path .. ".CR2"
    local image2_path_jpg = image2_path .. ".JPG"
    assert(io.open(image1_path_cr2, "rb") == nil)
    assert(io.open(image1_path_jpg, "rb") == nil)
    assert(io.open(image2_path_cr2, "rb") == nil)
    assert(io.open(image2_path_jpg, "rb") == nil)

    camera.burst(2)

    assert((dryos.shooting_card.file_number - initial_file_num) % 10000 == 2)
    camera.wait()
    local size1_cr2 = print_file_size(image1_path_cr2)
    local size1_jpg = print_file_size(image1_path_jpg)
    local size2_cr2 = print_file_size(image2_path_cr2)
    local size2_jpg = print_file_size(image2_path_jpg)
    assert(size1_cr2 or size1_jpg)
    assert(size2_cr2 or size2_jpg)
    dryos.image_prefix = ""     -- empty string restores the old prefix
    assert(dryos.image_prefix == old_prefix)

    -- let's try some more
    dryos.image_prefix = "XYZ_"
    assert(dryos.image_prefix == "XYZ_")
    dryos.image_prefix = "1234"
    assert(dryos.image_prefix == "1234")
    dryos.image_prefix = "$#@!"
    assert(dryos.image_prefix == "$#@!")
    dryos.image_prefix = ""
    assert(dryos.image_prefix == old_prefix)

    printf("Bracketed pictures...\n")
    initial_file_num = dryos.shooting_card.file_number
    camera.shutter.value = 1/500
    camera.shoot()
    camera.shutter.value = 1/50
    camera.shoot()
    camera.shutter.value = 1/5
    camera.shoot()
    assert((dryos.shooting_card.file_number - initial_file_num) % 10000 == 3)
    camera.wait()
    -- fixme: how to check metadata in the files?
    for i = dryos.shooting_card.file_number - 2, dryos.shooting_card.file_number do
        image_path = dryos.dcim_dir.path ..  dryos.image_prefix .. string.format("%04d", i % 10000)
        image_path_cr2 = image_path .. ".CR2"
        image_path_jpg = image_path .. ".JPG"
        local size_cr2 = print_file_size(image_path_cr2)
        local size_jpg = print_file_size(image_path_jpg)
        assert(size_cr2 or size_jpg)
    end

    sleep(2)

    printf("Bulb picture...\n")
    local t0 = dryos.ms_clock
    initial_file_num = dryos.shooting_card.file_number
    image_path = dryos.dcim_dir.path ..  dryos.image_prefix .. string.format("%04d", (initial_file_num + 1) % 10000)
    image_path_cr2 = image_path .. ".CR2"
    image_path_jpg = image_path .. ".JPG"
    assert(io.open(image_path_cr2, "rb") == nil)
    assert(io.open(image_path_jpg, "rb") == nil)

    camera.bulb(10)

    local t1 = dryos.ms_clock
    local elapsed = t1 - t0
    printf("Elapsed time: %s\n", elapsed)
    -- we can't measure this time accurately, so we only do a very rough check
    -- slow cards may be an issue, so let's allow a wide error margin
    assert(elapsed > 9900 and elapsed < 30000)
    assert((dryos.shooting_card.file_number - initial_file_num) % 10000 == 1)
    camera.wait()
    local size_cr2 = print_file_size(image_path_cr2)
    local size_jpg = print_file_size(image_path_jpg)
    assert(size_cr2 or size_jpg)

    printf("Picture taking tests completed.\n")
    printf("\n")
    sleep(5)
end

function test_lv()
    request_mode(MODE.M, "M")

    printf("Testing module 'lv'...\n")
    if lv.enabled then
        printf("LiveView is running; stopping...\n")
        lv.stop()
        assert(not lv.enabled, "LiveView did not stop")
        sleep(2)
    end

    assert(not lv.enabled)
    assert(lv.vidmode == "PH-NOLV")

    printf("Starting LiveView...\n")
    lv.start()
    assert(lv.enabled, "LiveView did not start")
    assert(not lens.autofocusing)
    assert(lv.vidmode == "PH-LV")
    assert(camera.gui.idle == true)

    sleep(2)

    -- lv.overlays status
    local function print_overlays_status()
        -- 1 = Canon overlays, 2 = ML overlays (when global draw is on), false = no overlays enabled
        -- using false instead of 0 to allow using in a conditional expression: if lv.overlays then ...
        printf("Overlays: %s\n", lv.overlays == 1 and "Canon" or lv.overlays == 2 and "ML" or "disabled");
    end

    console.hide(); assert(not console.visible)
    local old_gdr = menu.get("Overlay", "Global Draw")
    for i=1,10 do
        key.press(KEY.INFO)
        sleep(0.2); print_overlays_status()
        sleep(1)
        if lv.overlays ~= 1 then
            -- Canon overlays disabled?
            -- Enable ML overlays
            assert(menu.set("Overlay", "Global Draw", "ON"))
            sleep(0.2); print_overlays_status()
            assert(lv.overlays == 2)
            sleep(1)
            -- Disable ML overlays
            assert(menu.set("Overlay", "Global Draw", "OFF"))
            sleep(0.2); print_overlays_status()
            assert(lv.overlays == false)
            sleep(1)
        end
    end
    -- restore original Global Draw setting
    assert(menu.set("Overlay", "Global Draw", old_gdr))
    assert(menu.get("Overlay", "Global Draw") == old_gdr)
    sleep(0.2); print_overlays_status()
    console.show(); assert(console.visible)

    sleep(2)

    for i,z in pairs{1, 5, 10, 5, 1, 10, 1} do
        printf("Setting zoom to x%d...\n", z)
        lv.zoom = z
        assert(lv.zoom == z, "Could not set zoom in LiveView ")
        if z == 5 then
            assert(lv.vidmode == "ZOOM-X5");
        elseif z == 10 then
            assert(lv.vidmode == "ZOOM-X10");
        else
            assert(lv.vidmode == "PH-LV")
        end
        lv.wait(5)
    end

    printf("Pausing LiveView...\n")
    lv.pause()
    assert(lv.enabled, "LiveView stopped")
    assert(lv.paused, "LiveView could not be paused")
    assert(not lens.autofocusing)
    assert(lv.vidmode == "PAUSED-LV");

    sleep(2)

    printf("Resuming LiveView...\n")
    lv.resume()
    assert(lv.enabled, "LiveView stopped")
    assert(not lv.paused, "LiveView could not be resumed")
    assert(not lens.autofocusing)
    assert(lv.vidmode == "PH-LV");

    sleep(2)

    printf("Stopping LiveView...\n")
    lv.stop()

    assert(not lv.enabled, "LiveView did not stop")
    assert(not lv.paused,  "LiveView is disabled, can't be paused")
    assert(not lv.running, "LiveView is disabled, can't be running")
    assert(not lens.autofocusing)
    assert(lv.vidmode == "PH-NOLV")

    sleep(1)

    printf("LiveView tests completed.\n")
    printf("\n")
end

function test_lens_focus()
    printf("\n")
    printf("Testing lens focus functionality...\n")
    
    if lens.name == "" then
        printf("This test requires an electronic lens.\n")
        assert(not lens.af, "manual lenses can't autofocus")
        return
    end
    
    if not lens.af then
        printf("Please enable autofocus.\n")
        printf("(or, remove the lens from the camera to skip this test)\n")
        while not lens.af and lens.name ~= "" do
            console.show(); assert(console.visible)
            sleep(1)
        end
        sleep(1)
    end

    -- note: some lenses may be able to AF only in LiveView
    -- so let's check each mode regardless of the other

    if not lv.running then
        if lens.af then
            printf("Autofocus outside LiveView...\n")
            assert(lens.autofocus())
        end

        lv.start()
        assert(lv.running)
    end

    if lens.af then
        printf("Focus distance: %s\n",  lens.focus_distance)

        printf("Autofocus in LiveView...\n")
        assert(not lens.autofocusing)
        assert(lens.autofocus())
        assert(not lens.autofocusing)

        printf("Please trigger autofocus (half-shutter / AF-ON / * ).\n")
        for i = 1,2000 do
            msleep(10)
            if lens.autofocusing then
                printf("Autofocus triggered.\n")
                while lens.autofocusing do
                    msleep(10)
                end
                printf("Autofocus completed.\n")
                break
            end
            if i % 100 == 0 then
                printf("\b\b\b\b\b%d...", 20 - i // 100)
                io.flush()
            end
            if i // 100 == 20 then
                printf("\b\b\b\b\b")
                io.flush()
                assert(false, "Autofocus not triggered.\n")
            end
        end

        sleep(1)
        printf("Focus distance: %s\n",  lens.focus_distance)

        -- note: focus direction is not consistent
        -- some lenses will focus to infinity, others to macro
        printf("Focusing backward...\n")
        while lens.focus(-1,3,true) do end

        -- note: focus distance and position may not be reported right away
        sleep(0.5)

        printf("Focus distance: %s\n",  lens.focus_distance)
        printf("Focus motor position: %d\n", lens.focus_pos)
        
        for i,step in pairs{3,2,1} do
            for j,wait in pairs{true,false} do
                printf("Focusing forward with step size %d, wait=%s...\n", step, wait)
                local steps_front = 0
                local focus_pos_0 = lens.focus_pos
                while lens.focus(1,step,wait) do
                    printf(".")
                    steps_front = steps_front + 1
                end
                
                sleep(0.5)

                printf("\n")
                printf("Focus distance: %s\n",  lens.focus_distance)
                printf("Focus motor position: %d\n", lens.focus_pos)
                local focus_pos_1 = lens.focus_pos

                sleep(0.5)
                
                printf("Focusing backward with step size %d, wait=%s...\n", step, wait)
                local steps_back = 0
                while lens.focus(-1,step,wait) do
                    printf(".")
                    steps_back = steps_back + 1
                end
                
                sleep(0.5)

                printf("\n")
                printf("Focus distance: %s\n",  lens.focus_distance)
                printf("Focus motor position: %d\n", lens.focus_pos)
                local focus_pos_2 = lens.focus_pos

                sleep(0.5)

                local motor_steps_front = math.abs(focus_pos_1 - focus_pos_0)
                local motor_steps_back  = math.abs(focus_pos_2 - focus_pos_1)
                local motor_steps_lost  = math.abs(focus_pos_2 - focus_pos_0)
                printf("Focus range: %s steps forward, %s steps backward. \n",  steps_front, steps_back)
                printf("Motor steps: %s forward, %s backward, %s lost. \n",  motor_steps_front, motor_steps_back, motor_steps_lost)

                sleep(0.5)
            end
        end
        printf("\nFocus test completed.\n")
    else
        printf("Focus test skipped.\n")
    end
    printf("\n")
end

function test_movie()
    printf("\n")
    printf("Testing movie recording...\n")

    -- we must be in photo mode (from previous tests)
    assert(camera.mode ~= MODE.MOVIE)

    -- this should raise error
    local s,e = pcall(movie.start)
    assert(s == false)
    assert(e:find("movie mode"))

    request_mode(MODE.MOVIE, "Movie")
    lv.start()
    assert(lv.running)

    -- this should also raise error, as we are not exactly in LiveView
    lv.pause()
    local s,e = pcall(movie.start)
    assert(s == false)
    assert(e:find("LiveView"))
    menu.close()
    lv.resume()

    -- this should raise error, because the LiveView button has a special
    -- function in ML menu (so we can't just press it to start recording)
    menu.open()
    local s,e = pcall(movie.start)
    assert(s == false)
    assert(e:find("menu"))
    menu.close()

    -- now it should work
    -- hide the console for a nicer look
    console.hide(); assert(not console.visible)
    movie.start()
    assert(movie.recording)
    sleep(1)
    movie.stop()
    assert(not movie.recording)
    console.show(); assert(console.visible)

    -- let's review it
    assert(camera.gui.idle == true)
    camera.gui.play = true
    assert(camera.gui.play == true)
    assert(camera.gui.play_movie == true)
    assert(camera.gui.play_photo == false)

    printf("Movie recording tests completed.\n")
    printf("\n")
end

function api_tests()
    menu.close()
    console.clear()
    console.show()
    test_log = logger("LUATEST.LOG")

    -- note: each test routine must print a blank line at the end
    strict_tests()
    generic_tests()
    
    printf("Module tests...\n")
    test_io()
    test_camera_gui()
    test_menu()
    test_camera_take_pics()
    sleep(1)
    test_multitasking()
    test_keys()
    test_lv()
    test_lens_focus()
    test_camera_exposure()
    test_movie()
    
    printf("Done!\n")
    
    test_log:close()
    key.wait()
    console.hide()
end

-- check script arguments
assert(#arg == 0)
assert(arg[0] == "API_TEST.LUA" or arg[0] == "api_test.lua")

api_tests()

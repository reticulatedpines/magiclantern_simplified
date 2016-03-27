-- Test routines for the scripting API
-- Very incomplete
require("logger")

-- global logger
test_log = nil

function printf(s,...)
    test_log:writef(s,...)
    test_log:write("\n")
end

function request_mode(mode, mode_str)
    while camera.mode ~= mode do
        printf("Please switch to %s mode.", mode_str, mode)
        while camera.mode ~= mode do
            console.show()
            msleep(1000)
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
    printf("Strict mode tests...")

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

    printf("Strict mode tests passed.")
    printf("")
end

function generic_tests()
    printf("Generic tests...")
    print_table("camera")
    print_table("event")
    print_table("console")
    print_table("lv")
    print_table("lens")
    print_table("display")
    print_table("key")
    print_table("menu")
    print_table("testmenu")
    print_table("movie")
    print_table("dryos")
    print_table("interval")
    print_table("battery")
    print_table("task")
    print_table("property")
    printf("Generic tests completed.")
    printf("")
end

function test_camera_exposure()
    printf("Testing exposure settings, module 'camera'...")
    printf("Camera    : %s (%s) %s", camera.model, camera.model_short, camera.firmware)
    printf("Lens      : %s", lens.name)
    printf("Shoot mode: %s", camera.mode)
    printf("Shutter   : %s (raw %s, %ss, %sms, apex %s)", camera.shutter, camera.shutter.raw, camera.shutter.value, camera.shutter.ms, camera.shutter.apex)
    printf("Aperture  : %s (raw %s, f/%s, apex %s)", camera.aperture, camera.aperture.raw, camera.aperture.value, camera.aperture.apex)
    printf("Av range  : %s..%s (raw %s..%s, f/%s..f/%s, apex %s..%s)",
        camera.aperture.min, camera.aperture.max,
        camera.aperture.min.raw, camera.aperture.max.raw,
        camera.aperture.min.value, camera.aperture.max.value,
        camera.aperture.min.apex, camera.aperture.max.apex
    )
    printf("ISO       : %s (raw %s, %s, apex %s)", camera.iso, camera.iso.raw, camera.iso.value, camera.iso.apex)
    printf("EC        : %s (raw %s, %s EV)", camera.ec, camera.ec.raw, camera.ec.value)
    printf("Flash EC  : %s (raw %s, %s EV)", camera.flash_ec, camera.flash_ec.raw, camera.flash_ec.value)

    request_mode(MODE.M, "M")
    local old_value = camera.shutter.raw
    printf("Setting shutter to random values...")
    for k = 1,100 do
        local method = math.random(1,4)
        local d = nil
        if method == 1 then
            local s = math.random(1,30)
            camera.shutter.value = s
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
            printf("Error: shutter delta %s EV", d)
        end

        -- seconds and ms fields should be consistent
        if math.abs(camera.shutter.value - camera.shutter.ms/1000) > 1e-3 then
            printf("Error: shutter %ss != %sms", camera.shutter.value, camera.shutter.ms)
        end
        
        -- seconds and apex should be consistent
        -- see http://dougkerr.net/Pumpkin/articles/APEX.pdf
        local expected_apex = math.log(1/camera.shutter.value,2)
        if math.abs(expected_apex - camera.shutter.apex) > 0.01 then
            printf("Error: shutter %ss != Tv%s, expected %s", camera.shutter.value, camera.shutter.apex, expected_apex)
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
                printf("Error: shutter set to %s=%s, got %ss, expected %ss", field, current[field], camera.shutter.value, current.value)
            end
            if camera.shutter.ms ~= current.ms then
                printf("Error: shutter set to %s=%s, got %sms, expected %sms", field, current[field], camera.shutter.ms, current.ms)
            end
            if camera.shutter.apex ~= current.apex then
                printf("Error: shutter set to %s=%s, got Tv%s, expected Tv%s", field, current[field], camera.shutter.apex, current.apex)
            end
            if camera.shutter.raw ~= current.raw then
                printf("Error: shutter set to %s=%s, got %s, expected %s (raw)", field, current[field], camera.shutter.raw, current.raw)
            end
        end
    end
    camera.shutter.raw = old_value

    request_mode(MODE.M, "M")
    old_value = camera.iso.raw
    printf("Setting ISO to random values...")
    for k = 1,100 do
        local method = math.random(1,3)
        local d = nil
        if method == 1 then
            local iso = math.random(100, 6400)
            camera.iso.value = iso
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
            printf("Error: ISO delta %s EV", d)
        end

        -- ISO and Sv (APEX) should be consistent
        local expected_apex = math.log(camera.iso.value/3.125, 2)
        if math.abs(expected_apex - camera.iso.apex) > 0.2 then
            printf("Error: ISO %s != Sv%s, expected %s", camera.iso.value, camera.iso.apex, expected_apex)
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
                printf("Error: ISO set to %s=%s, got %s, expected %s", field, current[field], camera.iso.value, current.value)
            end
            if camera.iso.apex ~= current.apex then
                printf("Error: ISO set to %s=%s, got Sv%s, expected Sv%s", field, current[field], camera.iso.apex, current.apex)
            end
            if camera.iso.raw ~= current.raw then
                printf("Error: ISO set to %s=%s, got %s, expected %s (raw)", field, current[field], camera.iso.raw, current.raw)
            end
        end
    end
    camera.iso.raw = old_value

    if camera.aperture.min.raw == camera.aperture.max.raw then
        printf("This lens does not have variable aperture (skipping test).")
    else
        request_mode(MODE.M, "M")
        old_value = camera.aperture.raw
        printf("Setting aperture to random values...")
        for k = 1,100 do
            local method = math.random(1,3)
            local d = nil
            local extra_tol = 0
            if method == 1 then
                local av = math.random(round(camera.aperture.min.value*10), round(camera.aperture.max.value*10)) / 10
                camera.aperture.value = av
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
                printf("Error: aperture delta %s EV (expected < %s, f/%s, method=%d)", d, 1.5/8 + extra_tol, camera.aperture, method)
            end

            -- aperture and Av (APEX) should be consistent
            local expected_apex = math.log(camera.aperture.value, 2) * 2
            if math.abs(expected_apex - camera.aperture.apex) > 0.2 then
                printf("Error: aperture %s != Av%s, expected %s", camera.aperture.value, camera.aperture.apex, expected_apex)
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
                    printf("Error: aperture set to %s=%s, got %s, expected %s", field, current[field], camera.aperture.value, current.value)
                end
                if camera.aperture.apex ~= current.apex then
                    printf("Error: aperture set to %s=%s, got Sv%s, expected Sv%s", field, current[field], camera.aperture.apex, current.apex)
                end
                if camera.aperture.raw ~= current.raw then
                    printf("Error: aperture set to %s=%s, got %s, expected %s (raw)", field, current[field], camera.aperture.raw, current.raw)
                end
            end
        end
        camera.aperture.raw = old_value
    end

    request_mode(MODE.AV, "Av")
    old_value = camera.ec.raw
    printf("Setting EC to random values...")
    for k = 1,100 do
        local method = math.random(1,2)
        local d = nil
        if method == 1 then
            local ec = math.random(-2*100, 2*100) / 100
            camera.ec.value = ec
            d = math.abs(camera.ec.value - ec,2)
        elseif method == 2 then
            local raw = math.random(-16, 16)
            camera.ec.raw = raw
            d = math.abs(camera.ec.raw - raw) / 8
        end

        -- difference between requested and actual EC should be max 1.5/8 EV
        if d > 1.5/8 then
            printf("Error: EC delta %s EV", d)
        end

        -- EC and raw should be consistent
        local expected_ec = camera.ec.raw / 8
        if math.abs(expected_ec - camera.ec.value) > 0.001 then
            printf("Error: EC raw %s != %s EV, expected %s EV", camera.ec.raw, camera.ec.value, expected_ec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            local current = {}
            current.value = camera.ec.value
            current.raw   = camera.ec.raw
            
            camera.ec[field] = current[field]
            
            if camera.ec.value ~= current.value then
                printf("Error: EC set to %s=%s, got %s, expected %s EV", field, current[field], camera.ec.value, current.value)
            end
            if camera.ec.raw ~= current.raw then
                printf("Error: EC set to %s=%s, got %s, expected %s (raw)", field, current[field], camera.ec.raw, current.raw)
            end
        end
    end
    camera.ec.raw = old_value

    -- copy/paste & replace from EC (those two should behave in the same way)
    old_value = camera.flash_ec.raw
    printf("Setting Flash EC to random values...")
    for k = 1,100 do
        local method = math.random(1,2)
        local d = nil
        if method == 1 then
            local fec = math.random(-2*100, 2*100) / 100
            camera.flash_ec.value = fec
            d = math.abs(camera.flash_ec.value - fec,2)
        elseif method == 2 then
            local raw = math.random(-16, 16)
            camera.flash_ec.raw = raw
            d = math.abs(camera.flash_ec.raw - raw) / 8
        end

        -- difference between requested and actual EC should be max 1.5/8 EV
        if d > 1.5/8 then
            printf("Error: FEC delta %s EV", d)
        end

        -- EC and raw should be consistent
        local expected_fec = camera.flash_ec.raw / 8
        if math.abs(expected_fec - camera.flash_ec.value) > 0.001 then
            printf("Error: FEC raw %s != %s EV, expected %s EV", camera.flash_ec.raw, camera.flash_ec.value, current.expected_fec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            local current = {}
            current.value = camera.flash_ec.value
            current.raw   = camera.flash_ec.raw
            
            camera.flash_ec[field] = current[field]
            
            if camera.flash_ec.value ~= current.value then
                printf("Error: FEC set to %s=%s, got %s, expected %s EV", field, current[field], camera.flash_ec.value, current.value)
            end
            if camera.flash_ec.raw ~= current.raw then
                printf("Error: FEC set to %s=%s, got %s, expected %s (raw)", field, current[field], camera.flash_ec.raw, current.raw)
            end
        end
    end
    camera.flash_ec.raw = old_value

    printf("Exposure tests completed.")
    printf("")
end

function test_lv()
    printf("Testing module 'lv'...")
    if lv.enabled then
        printf("LiveView is running; stopping...");
        lv.stop()
        assert(not lv.enabled, "LiveView did not stop")
        msleep(2000)
    end

    printf("Starting LiveView...");
    lv.start()
    assert(lv.enabled, "LiveView did not start")

    msleep(2000)
    
    for i,z in pairs{1, 5, 10, 5, 1, 10, 1} do
        printf("Setting zoom to x%d...", z)
        lv.zoom = z
        assert(lv.zoom == z, "Could not set zoom in LiveView ")
        lv.wait(5)
    end

    printf("Pausing LiveView...");
    lv.pause()
    assert(lv.enabled, "LiveView stopped")
    assert(lv.paused, "LiveView could not be paused")

    msleep(2000)

    printf("Resuming LiveView...");
    lv.resume()
    assert(lv.enabled, "LiveView stopped")
    assert(not lv.paused, "LiveView could not be resumed")

    msleep(2000)

    printf("Stopping LiveView...");
    lv.stop()

    assert(not lv.enabled, "LiveView did not stop")
    assert(not lv.paused,  "LiveView is disabled, can't be paused")
    assert(not lv.running, "LiveView is disabled, can't be running")

    msleep(1000)

    printf("LiveView tests completed.");
    printf("")
end

function api_tests()
    menu.close()
    console.clear()
    console.show()
    test_log = logger("LUATEST.LOG")

    strict_tests()
    generic_tests()
    
    printf("Module tests...")
    test_camera_exposure()
    test_lv()
    
    printf("Done!")
    
    test_log:close()
    key.wait()
    console.hide()
end

testmenu = menu.new
{
    name = "Script API tests",
    select = function(this) task.create(api_tests) end,
}


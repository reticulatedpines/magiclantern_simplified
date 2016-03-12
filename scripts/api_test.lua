-- Test routines for the scripting API
-- Very incomplete

function printf(s,...)
    -- fixme: newlines (io.write doesn't work)
    print(s:format(...))
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

function api_tests()
    menu.close()
    console.clear()
    console.show()
    printf("Testing module 'camera'...")
    printf("Camera    : %s (%s) %s", camera.model, camera.model_short, camera.firmware)
    printf("Shoot mode: %s", camera.mode)
    printf("Shutter   : %s (raw %s, %ss, %sms, apex %s)", camera.shutter, camera.shutter.raw, camera.shutter.value, camera.shutter.ms, camera.shutter.apex)
    printf("Aperture  : %s (raw %s, 1/%s, apex %s)", camera.aperture, camera.aperture.raw, camera.aperture.value, camera.aperture.apex)
    printf("ISO       : %s (raw %s, %s, apex %s)", camera.iso, camera.iso.raw, camera.iso.value, camera.iso.apex)
    printf("EC        : %s (raw %s, %s EV)", camera.ec, camera.ec.raw, camera.ec.value)
    printf("Flash EC  : %s (raw %s, %s EV)", camera.flash_ec, camera.flash_ec.raw, camera.flash_ec.value)
    printf("Kelvin    : %s", camera.kelvin)
    
    request_mode(MODE.M, "M")
    printf("Setting shutter to random values...")
    for k = 1,100 do
        method = math.random(1,4)
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
        expected_apex = math.log(1/camera.shutter.value,2)
        if math.abs(expected_apex - camera.shutter.apex) > 0.01 then
            printf("Error: shutter %ss != Tv%s, expected %s", camera.shutter.value, camera.shutter.apex, expected_apex)
        end
        
        -- setting shutter to the same value, using any method (s,ms,apex,raw)
        -- should not change anything
        for i,field in pairs{"value","ms","apex","raw"} do
            apex  = camera.shutter.apex
            value = camera.shutter.value
            ms    = camera.shutter.ms
            raw   = camera.shutter.raw
            
            -- ms is integer, so tests at very low values will fail because of roundoff errors
            -- however, if we have set the shutter in ms from the beginning, it will work fine
            if field == "ms" and ms < 10 and method ~= 2 then
                -- how do you write "continue" in Lua?!
                field = "value"
            end
            
            camera.shutter[field] = _G[field]
            
            if camera.shutter.value ~= value then
                printf("Error: shutter set to %s=%s, got %ss, expected %ss", field, _G[field], value, camera.shutter.value, value)
            end
            if camera.shutter.ms ~= ms then
                printf("Error: shutter set to %s=%s, got %sms, expected %sms", field, _G[field], camera.shutter.ms, ms)
            end
            if camera.shutter.apex ~= apex then
                printf("Error: shutter set to %s=%s, got Tv%s, expected Tv%s", field, _G[field], camera.shutter.apex, apex)
            end
            if camera.shutter.raw ~= raw then
                printf("Error: shutter set to %s=%s, got %s, expected %s (raw)", field, _G[field], camera.shutter.raw, raw)
            end
        end
    end

    request_mode(MODE.M, "M")
    printf("Setting ISO to random values...")
    for k = 1,100 do
        method = math.random(1,3)
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
        expected_apex = math.log(camera.iso.value/3.125, 2)
        if math.abs(expected_apex - camera.iso.apex) > 0.2 then
            printf("Error: ISO %s != Sv%s, expected %s", camera.iso.value, camera.iso.apex, expected_apex)
        end

        -- setting ISO to the same value, using any method (value,apex,raw)
        -- should not change anything
        for i,field in pairs{"value","apex","raw"} do
            apex  = camera.iso.apex
            value = camera.iso.value
            raw   = camera.iso.raw
            
            camera.iso[field] = _G[field]
            
            if camera.iso.value ~= value then
                printf("Error: ISO set to %s=%s, got %s, expected %s", field, _G[field], value, camera.iso.value, value)
            end
            if camera.iso.apex ~= apex then
                printf("Error: ISO set to %s=%s, got Sv%, expected Sv%", field, _G[field], camera.iso.apex, apex)
            end
            if camera.iso.raw ~= raw then
                printf("Error: ISO set to %s=%s, got %s, expected %s (raw)", field, _G[field], camera.iso.raw, raw)
            end
        end
    end

    request_mode(MODE.AV, "Av")
    printf("Setting EC to random values...")
    for k = 1,100 do
        method = math.random(1,2)
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
        expected_ec = camera.ec.raw / 8
        if math.abs(expected_ec - camera.ec.value) > 0.001 then
            printf("Error: EC raw %s != %s EV, expected %s EV", camera.ec.raw, camera.ec.value, expected_ec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            value = camera.ec.value
            raw   = camera.ec.raw
            
            camera.ec[field] = _G[field]
            
            if camera.ec.value ~= value then
                printf("Error: EC set to %s=%s, got %s, expected %s EV", field, _G[field], value, camera.ec.value, value)
            end
            if camera.ec.raw ~= raw then
                printf("Error: EC set to %s=%s, got %s, expected %s (raw)", field, _G[field], camera.ec.raw, raw)
            end
        end
    end

    -- copy/paste & replace from EC (those two should behave in the same way)
    printf("Setting Flash EC to random values...")
    for k = 1,100 do
        method = math.random(1,2)
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
        expected_fec = camera.flash_ec.raw / 8
        if math.abs(expected_fec - camera.flash_ec.value) > 0.001 then
            printf("Error: FEC raw %s != %s EV, expected %s EV", camera.flash_ec.raw, camera.flash_ec.value, expected_fec)
        end

        -- setting EC to the same value, using any method (value,raw)
        -- should not change anything
        for i,field in pairs{"value","raw"} do
            value = camera.flash_ec.value
            raw   = camera.flash_ec.raw
            
            camera.flash_ec[field] = _G[field]
            
            if camera.flash_ec.value ~= value then
                printf("Error: FEC set to %s=%s, got %s, expected %s EV", field, _G[field], value, camera.flash_ec.value, value)
            end
            if camera.flash_ec.raw ~= raw then
                printf("Error: FEC set to %s=%s, got %s, expected %s (raw)", field, _G[field], camera.flash_ec.raw, raw)
            end
        end
    end

    printf("Done!")

    key.wait()
    console.hide()
end

testmenu = menu.new
{
    name = "Script API tests",
    select = function(this) task.create(api_tests) end,
}


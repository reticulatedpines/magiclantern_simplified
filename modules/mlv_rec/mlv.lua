
-- 
-- mlv_dump LUA example script
-- 
-- The LUA scripting language is *not* made for efficient bit and byte modifications, so it is hard to patch bytes here.
-- But it is very easy to embed into existing C programs and has a stable API.
-- 

--
-- helpers
--
function replace_bytes(pos, str, hex)
    local binary = hex:fromhex();
    return str:sub(1, pos-1) .. binary .. str:sub(pos + binary:len())
end

function string.fromhex(str)
    return (str:gsub('..', function (cc)
        return string.char(tonumber(cc, 16))
    end))
end

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end
--
--


function init()
    print("LUA script loaded");
end

function dng_saved(file, seq)
    print("  --  DNG file was saved (file: '"..file.."', seq: '"..seq.."')");
end

function handle_MLVI(hdr)
    print("  --  handle MLVI ("..#hdr.." byte)");
end

function handle_VIDF(hdr)
    print("  --  handle VIDF ("..#hdr.." byte)");
end

function handle_VIDF_data_read(hdr, data)
    print("  --  handle VIDF after read. hdr ("..#hdr.." byte) and data ("..#data.." byte)");
end

function handle_VIDF_data_write(hdr, data)
    print("  --  handle VIDF before write. hdr ("..#hdr.." byte) and data ("..#data.." byte)");
end

function handle_VIDF_data_write_mlv(hdr, data)
    print("  --  handle VIDF before write as MLV. hdr ("..#hdr.." byte) and data ("..#data.." byte)");
end

function handle_RTCI(hdr)
    print("  --  handle RTCI ("..#hdr.." byte)");
    
    -- replace data in header at position 17 (counting from 1) with 0xDEAD in little endian
    -- this is the tm_sec field
    local ret = replace_bytes(17, hdr, "ADDE");
    
    return ret;
end


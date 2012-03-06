PROP_WB_MODE_LV = 0x80050018
PROP_WB_KELVIN_LV = 0x80050019
PROP_WB_MODE_PH = 0x8000000D
PROP_WB_KELVIN_PH = 0x8000000E

setprop(PROP_WB_MODE_LV,9);
setprop(PROP_WB_MODE_PH,9);

setwb = function(wb)
  setprop(PROP_WB_KELVIN_LV, wb);
  setprop(PROP_WB_KELVIN_PH, wb);
end

for wb = 3000, 6000, 200 do
  setwb(wb)
  cprint("Shooting at "..wb.."K\n")
  shoot(64, false)
  msleep(1000)
end

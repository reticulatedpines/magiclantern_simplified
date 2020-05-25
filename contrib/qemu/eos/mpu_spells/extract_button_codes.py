#!/usr/bin/python

from __future__ import print_function
from unicorn import *
from unicorn.arm_const import *
import os, sys, re
from outils import *

# load ROM1
qemu_dir = "../../../../"
camera_model = sys.argv[1]
ROM = open(os.path.join(qemu_dir, "%s/ROM1.BIN" % camera_model)).read()
rom_size = len(ROM);
rom_offset = 0x100000000 - rom_size;
print("ROM:", hex(rom_offset), hex(rom_size))

rom_funcs = [
    ("bindReceiveSwitch",   "bindReceiveSwitch (%d, %d)",   2, 0xE, 0),
    ("DebugMsg",            "bindReceiveSwitch (%d, %d)",   2, 0xE, 1),
    ("prop_request_change", "pRequestChange",               0, 0x0, 0),
]

if camera_model in ["450D", "40D"]:
    rom_funcs = [
        ("bindReceiveSwitch",   "[BIND] Switch (%d, %d)",   2, 0xE, 0),
        ("DebugMsg",            "[BIND] Switch (%d, %d)",   2, 0xE, 1),
        ("prop_request_change", "pRequestChange",           0, 0xE, 0),
    ]

for name, string, reg, cond, idx in rom_funcs:
    addr = (find_func_from_string(ROM, string, reg, cond)[idx] + rom_offset) & 0xFFFFFFFF
    eprint("%-20s: %X" % (name, addr))
    exec("%s = 0x%X" % (name, addr))

switch_names = get_switch_names(camera_model)
eprint(switch_names)

# some helper routines
def mem_read32(mu, addr):
    return getLongLE(mu.mem_read(addr, 4), 0)

def init_emulator():
    # Initialize emulator in ARM 32-bit mode
    mu = Uc(UC_ARCH_ARM, UC_MODE_ARM)

    # map ROM (variable size) and 16MB of RAM
    mu.mem_map(rom_offset, rom_size, UC_PROT_READ | UC_PROT_EXEC)
    mu.mem_map(0, 16 * 1024 * 1024)
    mu.mem_write(rom_offset, ROM)

    # patch prop_request_change
    bx_lr = b"\x1e\xff\x2f\xe1"
    mu.mem_write(prop_request_change, bx_lr)

    # init SP and LR
    mu.reg_write(UC_ARM_REG_SP, 0x1900)
    mu.reg_write(UC_ARM_REG_LR, 0xABCD0000)
    
    return mu

def print_DebugMsg(mu):
    pc = mu.reg_read(UC_ARM_REG_PC)
    sp = mu.reg_read(UC_ARM_REG_SP)
    r2 = mu.reg_read(UC_ARM_REG_R2)
    r3 = mu.reg_read(UC_ARM_REG_R3)
    assert(pc == DebugMsg)
    msg = str(mu.mem_read(r2, 256).split("\0")[0]);
    num_args = msg.count("%")
    args = []
    if num_args: args.append(r3)
    while len(args) < num_args:
        args.append(mem_read32(mu, sp + (len(args)-1) * 4))
    try: msg = msg % tuple(args)
    except ValueError: pass
    eprint("DebugMsg: " + msg)
    return msg

name_to_mpu = {}
mpu_to_name = {}

# start from scratch
mu = init_emulator()
mu_dirty = False

def try_button_code(a, b):
    global mu, mu_dirty
    
    if mu_dirty:
        # re-init emulator on error
        # fixme: memory leak (the object cleans up, but the ROM contents appear to leak)
        # workaround: keep the ROM small and only re-init emulator on error
        mu = init_emulator()

    
    # pass arguments to bindReceiveSwitch
    mu.reg_write(UC_ARM_REG_R0, a)
    mu.reg_write(UC_ARM_REG_R1, b)

    # emulate bindReceiveSwitch until DebugMsg prints something about GUI_Control
    start = bindReceiveSwitch
    while True:
        try:
            mu.emu_start(start, DebugMsg, timeout=100000)
        except UcError as e:
            pc = mu.reg_read(UC_ARM_REG_PC)
            lr = mu.reg_read(UC_ARM_REG_PC)
            eprint("%08X from %08X: %s" % (pc, lr, e))
            mu_dirty = True
            break
        
        pc = mu.reg_read(UC_ARM_REG_PC)

        if pc == 0xABCD0000:
            break
        
        msg = print_DebugMsg(mu) if pc == DebugMsg else ""
        
        if "GUI_Control" in msg:
            button_code = mu.reg_read(UC_ARM_REG_R3)
            button_name = switch_names.get(button_code, None)
            if button_name:
                eprint("Switch(%d, %d) -> %s" % (a, b, button_name))
                name_to_mpu[button_name] = (a,b)
                mpu_to_name[a,b] = button_name
            else:
                eprint("Switch(%d, %d) -> ?!" % (a, b))
            break
        
        if "Unknown DIRECTION" in msg:
            return True
        
        start =  mu.reg_read(UC_ARM_REG_LR)

for x in range(40):
    if try_button_code(x, 0xFF):
        for i in range(10):
            try_button_code(x, i)
    try_button_code(x, 0)
    try_button_code(x, 1)

# GMT_GUICMD_OPEN_BATT_COVER is harder to find,
# but consistent with GMT_GUICMD_OPEN_SLOT_COVER
name_to_mpu["GMT_GUICMD_OPEN_BATT_COVER"] = tuple(map(sum,
    zip(name_to_mpu["GMT_GUICMD_OPEN_SLOT_COVER"], (1, 0))))

print("static int button_codes_%s[] = {" % camera_model)
for n,v in sorted(name_to_mpu.iteritems()):
    print("    %-35s = 0x%02X%02X," % ("[%s]" % n, v[0], v[1]))
print("    [BGMT_END_OF_LIST]                  = 0x0000")
print("};")
print("")

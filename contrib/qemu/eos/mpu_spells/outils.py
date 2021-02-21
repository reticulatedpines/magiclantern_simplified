from __future__ import print_function
import os, sys, re
from struct import unpack

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

# button names from gui.h
def get_switch_names(camera_model):
    switch_names = {}
    ml_dir = "../../../../../magic-lantern/platform/"
    
    # pick one platform directory for this camera (any firmware version)
    cam_dir = [d for d in os.listdir(ml_dir) 
                 if d.split(".")[0] == camera_model
                 and os.path.isfile(os.path.join(ml_dir, d, "gui.h"))
              ][0]
    
    gui_h = open(os.path.join(ml_dir, cam_dir, "gui.h")).readlines()
    for l in gui_h:
        m = re.match(" *#define +([A-Z0-9_]+)[ \t]+([0-9a-fA-Fx]+)", l)
        if m:
            btn_name = m.groups()[0]
            btn_code = m.groups()[1]
            try: btn_code = int(btn_code,16) if btn_code.startswith("0x") else int(btn_code)
            except: continue
            
            # fixme: some unused button codes are declared as 0
            # (which is a valid code: BGMT_WHEEL_UP)
            if btn_code == 0 and btn_name != "BGMT_WHEEL_UP":
                eprint("FIXME: %s = 0" % btn_name)
                continue
            
            # this is a different event type
            if btn_name == "GMT_LOCAL_DIALOG_REFRESH_LV":
                continue
            
            switch_names[btn_code] = btn_name
    return switch_names

def ror(word, count):
    return (word >> count | word << (32 - count)) & 0xFFFFFFFF

def decode_immediate_shifter_operand(insn):
    inmed_8 = insn & 0xFF
    rotate_imm = (insn & 0xF00) >> 7
    return ror(inmed_8, rotate_imm)

def locate_func_start(ROM, search_from):
    # search backwards until a function start
    STMFD = 0xe92d0000
    for i in range(search_from, 0, -4):
        insn = getLongLE(ROM, i)
        if (insn & 0xFFFF0000) == (STMFD & 0xFFFF0000):
            insn2 = getLongLE(ROM, i-4)
            if (insn2 & 0xFFFF0000) == (STMFD & 0xFFFF0000):
                return i-4
            return i;

def locate_next_func_call(ROM, search_from):
    for pc in range(search_from, search_from + 0x100, 4):
        insn = getLongLE(ROM, pc)
        # BL instruction
        if (insn & 0x0F000000) == 0x0B000000:
            # fixme: handle negative jumps
            #assert((insn & 0x00800000) == 0)
            return (((insn & 0x00FFFFFF) << 2) + pc + 8) & 0xFFFFFFFF

def find_func_from_string(ROM, string, Rd, cond):
    string += "\0"
    for i in range(0, len(ROM), 4):
        insn = getLongLE(ROM, i)
        # check for: add<cond> Rd, pc, #offset
        if (insn & 0xFFFFF000) == (0x028f0000 | (Rd << 12) | (cond << 28)):
            # let's check if it refers to our string
            offset = decode_immediate_shifter_operand(insn);
            pc = i;
            dest = pc + offset + 8;
            dest_str = ROM[dest:dest+len(string)]
            if dest_str == string:
                return locate_func_start(ROM, i), locate_next_func_call(ROM, i)

        # check for: ldr<cond> Rd, [pc, #offset]
        if (insn & 0xFFFFF000) == (0x059f0000 | (Rd << 12) | (cond << 28)):
            offset = insn & 0xFFF;
            pc = i;
            dest = pc + offset + 8;
            dest = getLongLE(ROM, dest) - 0xFF800000  # fixme
            if dest > 0 and dest < len(ROM):
                dest_str = ROM[dest:dest+len(string)]
                if dest_str == string:
                    return locate_func_start(ROM, i), locate_next_func_call(ROM, i)

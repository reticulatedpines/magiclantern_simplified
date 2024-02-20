import os, sys, re
from collections import defaultdict

import logging, argparse
from logging import debug, info
parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action="store_const", dest="loglevel", const=logging.INFO)
parser.add_argument("-d", "--debug", action="store_const", dest="loglevel", const=logging.DEBUG)
args = parser.parse_args()
logging.basicConfig(level=args.loglevel, format='%(message)s')

lines = sys.stdin.readlines()

# keep track of call stack from log lines
callstacks = defaultdict(list)
stackid = None

# call stack printed before each debug message
printed_stack = None

# some stats
call_stacks = 0
task_switches = 0
interrupts = 0
warnings = 0

def match_stacks(current_stack, printed_stack):
    global warnings
    debug("".join(current_stack))
    debug("".join(printed_stack))
    for l, p in zip(current_stack, printed_stack):
        l = l.replace("call 0x", "0x")

        at_l = re.search(" at \[(.+):(.+):(.+)\]", l)
        at_p = re.search(" at \[(.+):(.+):(.+)\]", p)
        if at_l and at_p:   # fixme: long lines have this on the next line
            task_l, lr_l = at_l.groups()[:2]
            task_p, lr_p = at_p.groups()[:2]
            # fixme: interrupt ID is misreported
            if task_l.startswith("INT-"): task_l = "INT"
            if task_p.startswith("INT-"): task_p = "INT"
            assert task_l == task_p
            assert lr_l == lr_p

        if " at [" in l:
            l = l[:l.index("at [")].rstrip()
        if " at [" in p:
            p = p[:p.index("at [")].rstrip()
        
        if l[:70].rstrip() != p[:70].rstrip():
            warnings += 1
            print l
            print p
            # only report different arguments as warnings (fixme)
            # but don't allow differences on what functions were called
            if l.strip().split("(")[0] != p.strip().split("(")[0]:
                assert False

startup_workaround = "Task switch to Startup2" not in "".join(lines)
debug("startup_workaround = %s", startup_workaround)

for line in lines:
    info(line.strip("\n"))

    current_stack = callstacks[stackid]

    if printed_stack != None:
        if line.strip().startswith("0x") or line.strip().startswith("interrupt "):
            printed_stack.append(line)
        elif line.startswith(" " * 80 + " at "):
            pass
        else:
            assert line[0] == "["
            if not startup_workaround or stackid != "Startup":
                assert current_stack == callstacks[stackid]
                match_stacks(current_stack, printed_stack)
            printed_stack = None

    elif "Current stack" in line:
        printed_stack = []
        call_stacks += 1
        continue

    elif "Task switch" in line:
        task_switches += 1
        m = re.search(" to ([^: ]*):", line)
        assert m
        if stackid != "interrupt":
            stackid = m.groups()[0]
            debug("task switch -> %s", stackid)

    elif "return from interrupt" in line:
        level = line.index("return ")
        assert level < len(current_stack)
        current_stack = callstacks[stackid] = current_stack[:level]
        if level == 0:
            m = re.search(" at \[([^: ]*):", line)
            assert m
            stackid = m.groups()[0]
            debug("reti -> %s", stackid)
        continue

    elif line.strip().startswith("interrupt "):
        stackid = "interrupt"
        callstacks[stackid].append(line)
        interrupts += 1

    elif startup_workaround and stackid == "Startup":
        # fixme: some cameras start two different tasks with the same name
        # this confuses our checks; skip it for now
        pass

    elif "call " in line:
        level = line.index("call ")
        assert level == len(current_stack)
        current_stack.append(line)

    elif "return " in line:
        level = line.index("return ")
        assert level < len(current_stack)
        current_stack = callstacks[stackid] = current_stack[:level]

    elif " -> " in line:
        # direct jump
        m = re.match(" *-> ([^ ]* [a-zA-Z0-9_]*) +at ", line)
        if m:
            # fixme: this assumes only the last function in the jump chain is named
            jump_target = m.groups()[0].strip()
            current_stack[-1] = current_stack[-1].replace("(", " -> %s(" % jump_target, 1)

    # for all lines:
    debug("<stack '%s', %d items>", stackid, len(current_stack))

    # check context info from each line (must match the task switch info)
    # fixme: currently, context info is printed for the new (updated) state
    # e.g.  task switch to init:... at [init:...]
    # or:   interrupt 10h ... at [INT-10:...]
    # todo: task switch to foobar:... at [old_task:...]
    #       interrupt 10h ... at [old_task:...]
    at = re.search(" at \[(.+):(.+):(.+)\]", line)
    if at:
        at_task, at_a1, at_a2 = at.groups()
        debug("<%s:%s:%s>", at_task, at_a1, at_a2)
        assert stackid == ("interrupt" if at_task.startswith("INT-") else at_task)

print "%d stack traces, %d stacks, %d task switches, %d interrupts" % (call_stacks, len(callstacks.keys()), task_switches, interrupts)
if warnings:
    print "%d warning(s)" % warnings

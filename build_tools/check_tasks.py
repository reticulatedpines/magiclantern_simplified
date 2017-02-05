# Tag each ML function with the task(s) where it's called from.
#
# Uses static analysis (pycparser) and identifies common idioms
# from ML code (TASK_CREATE, PROP_HANDLER, menu declarations etc).
#
# Should work with both Python 2 and 3.

from __future__ import print_function
import os, sys, re
from collections import defaultdict
from pycparser import c_parser, c_ast, parse_file
from copy import copy, deepcopy

# copied from build log (5D3 1.1.3)
# fixme: extract from Makefile
source_files = [
    "src/boot-hack.c",
    "src/fio-ml.c",
    "src/mem.c",
    "src/ico.c",
    "src/edmac.c",
    "src/menu.c",
    "src/debug.c",
    "src/rand.c",
    "src/posix.c",
    "src/util.c",
    "src/imath.c",
    "src/electronic_level.c",
    "platform/5D3.113/cfn.c",
    "src/gui.c",
    "src/picstyle.c",
    "src/exmem.c",
    "src/bmp.c",
    "src/rbf_font.c",
    "src/config.c",
    "src/stdio.c",
    "src/bitrate-5d3.c",
    "src/lcdsensor.c",
    "src/tweaks.c",
    "src/tweaks-eyefi.c",
    "src/lens.c",
    "src/property.c",
    "src/propvalues.c",
    "src/gui-common.c",
    "src/chdk-gui_draw.c",
    "src/movtweaks.c",
    "src/menuhelp.c",
    "src/menuindex.c",
    "src/focus.c",
    "src/notify_box.c",
    "src/bootflags.c",
    "src/dialog_test.c",
    "src/vram.c",
    "src/greenscreen.c",
    "src/fps-engio.c",
    "src/shoot.c",
    "src/hdr.c",
    "src/lv-img-engio.c",
    "src/state-object.c",
    "src/tasks.c",
    "src/vsync-lite.c",
    "src/tskmon.c",
    "src/battery.c",
    "src/imgconv.c",
    "src/histogram.c",
    "src/falsecolor.c",
    "src/audio-ak.c",
    "src/zebra.c",
    "src/vectorscope.c",
    "src/beep.c",
    "src/crop-mode-hack.c",
    "src/ph_info_disp.c",
    "src/flexinfo.c",
    "src/screenshot.c",
    "src/fileprefix.c",
    "src/lvinfo.c",
    "src/builtin-enforcing.c",
    "src/powersave.c",
    "src/ml-cbr.c",
    "src/raw.c",
    "src/chdk-dng.c",
    "src/edmac-memcpy.c",
    "src/console.c",
    "src/tcc-glue.c",
    "src/module.c",
    "src/video_hacks.c",
    "src/afma.c",
    "src/asm.c",
]

# fixme: extract from Makefile, fix Lua
module_files = [
    "modules/raw_rec/raw_rec.c",
    "modules/mlv_play/mlv_play.c",
    "modules/mlv_rec/mlv_rec.c",
    "modules/mlv_rec/mlv.c",
    "modules/mlv_snd/mlv_snd.c",
    "modules/file_man/file_man.c",
    "modules/pic_view/pic_view.c",
    "modules/ettr/ettr.c",
    "modules/dual_iso/dual_iso.c",
    "modules/silent/silent.c",
    "modules/dot_tune/dot_tune.c",
    "modules/autoexpo/autoexpo.c",
    "modules/arkanoid/arkanoid.c",
    "modules/deflick/deflick.c",
    #~ "modules/lua/lua_globals.c",
    #~ "modules/lua/lua_console.c",
    #~ "modules/lua/lua_camera.c",
    #~ "modules/lua/lua_lv.c",
    #~ "modules/lua/lua_lens.c",
    #~ "modules/lua/lua_movie.c",
    #~ "modules/lua/lua_display.c",
    #~ "modules/lua/lua_key.c",
    #~ "modules/lua/lua_menu.c",
    #~ "modules/lua/lua_dryos.c",
    #~ "modules/lua/lua_interval.c",
    #~ "modules/lua/lua_battery.c",
    #~ "modules/lua/lua_task.c",
    #~ "modules/lua/lua_property.c",
    #~ "modules/lua/lua_constants.c",
    #~ "modules/lua/lua.c",
    "modules/bench/bench.c",
    "modules/selftest/selftest.c",
    "modules/adv_int/adv_int.c",
]

# troubleshooting
if 0:
    source_files = ["src/menu.c"]
    module_files = ["modules/adv_int/adv_int.c"]

def parse_src(filename):
    ast = parse_file(
        filename, use_cpp=True,
        cpp_path="arm-none-eabi-cpp",
        cpp_args=[
            '-Isrc', 
            '-Iplatform/5D3.113',
            '-Iplatform/5D3.113/include',
            '-D__extension__=',
            '-D__attribute__(x)=',
            '-D__builtin_va_list=int',
            '-D_GNU_SOURCE',
            '-DPYCPARSER',
            '-DCONFIG_MAGICLANTERN=1',
            '-DCONFIG_5D3=1',
            '-DRESTARTSTART=0',
            '-DROMBASEADDR=0',
            '-DCONFIG_CONSOLE',
            '-DCONFIG_TCC',
            '-DCONFIG_MODULES',
            '-DCONFIG_MODULES_MODEL_SYM="5D3.sym"'
        ]
    )
    return ast

def parse_module(filename):
    mod_name = filename.split("/")[-2]
    ast = parse_file(
        filename, use_cpp=True,
        cpp_path="arm-none-eabi-cpp",
        cpp_args=[
            '-Isrc', 
            '-Iplatform/all', 
            '-Imodules/%s' % mod_name, 
            '-D__extension__=',
            '-D__attribute__(x)=',
            '-D__builtin_va_list=int',
            '-D_GNU_SOURCE',
            '-DPYCPARSER',
            '-DMODULE',
            '-DMODULE_NAME=%s' % mod_name,
            '-DCONFIG_MAGICLANTERN',
        ]
    )
    return ast

# globals
current_file = "foo/bar.c"
is_func = defaultdict(bool)
func_list = []
func_calls = defaultdict(list)
func_calls_loc = defaultdict(list)
func_storage = defaultdict(str)
func_tasks = defaultdict(list)

def localized_func_name(func_name):
    # if a static function is found in current file,
    # assume the call is local (within that file)
    # prepend the file name to get a unique name
    func_name_local = current_file + ":" + func_name
    if is_func[func_name_local]:
        func_name = func_name_local
    return func_name

def friendly_func_name(func_name):
    """
    >>> friendly_func_name("main")
    'main'
    >>> friendly_func_name("foo/bar/baz.c:print")
    'baz.c:print'
    >>> friendly_func_name("foo/bar.c:print")
    'print'
    >>> friendly_func_name("src/bar.c:print")
    'bar.c:print'
    """
    if func_name.startswith(current_file + ":"):
        return func_name[len(current_file + ":"):]
    if "/" in func_name:
        return func_name[func_name.rfind('/')+1:]
    return func_name

def add_task(func_name, task_name):
    func_tasks[localized_func_name(func_name)].append(task_name)

# we only know the start line for each function, but not the end
# assume each node belongs to the last defined function
# (should work fine as long as the node actually belongs to a function)
def lookup_caller(node):
    caller = None
    line = node.coord.line
    for n,s,l in sorted(func_list, key=lambda x: x[1]):
        if s == node.coord.file:
            if l <= line:
                caller = n
    assert(caller)
    return caller

def add_func_call(func, verbose=False):
    func_name = localized_func_name(func.name)
    caller = lookup_caller(func)

    if func_name != caller:
        func_calls[func_name].append(caller)
        func_calls_loc[func_name].append(str(func.coord))

    if verbose:
        action = "called" if type(func) == c_ast.FuncCall else "referenced"
        if caller == func.name:
            action = "self-" + action
        print('%s %s at %s (%s)' % (func.name, action, func.coord, caller))

def parse_menu_entries(entries):
    assert type(entries) == c_ast.InitList
    for entry in entries.children():
        assert type(entry[1]) == c_ast.InitList
        for field in entry[1].children():
            assert type(field[1]) == c_ast.NamedInitializer
            field_name = field[1].children()[1][1].name
            if field_name in ["name"]:
                if type(field[1].children()[0][1]) == c_ast.Constant:
                    entry_name = field[1].children()[0][1].value
                    print("  menu_entry", entry_name)
                else:
                    print("  menu_entry", type(field[1].children()[0][1]))
            if field_name in ["select", "select_Q"]:
                func_name = field[1].children()[0][1].name
                add_task(func_name, "gui_main_task")
            if field_name in ["update"]:
                func_name = field[1].children()[0][1].name
                add_task(func_name, "menu_redraw_task")
            if field_name in ["children"]:
                children = field[1].children()[0][1]
                if type(children) == c_ast.CompoundLiteral:
                    assert type(children.children()[0][1]) == c_ast.Typename
                    assert type(children.children()[1][1]) == c_ast.InitList
                    parse_menu_entries(children.children()[1][1])

class StructVisitor(c_ast.NodeVisitor):
    def visit_Decl(self, node):
        if node.name:
            if node.name.startswith("task_create_"):
                args = node.children()[1][1].children()
                if len(args) == 2:
                    task_name = "ml_init"
                    func_name = args[1][1].expr.name
                    print("  INIT_FUNC", func_name)
                elif len(args) == 5:
                    task_name = args[0][1].expr.value.strip('"')
                    func_name = args[1][1].expr.name
                    print("  TASK_CREATE", task_name, func_name)
                else:
                    assert 0
                add_task(func_name, task_name)

            elif node.name.startswith("task_mapping_"):
                args = node.children()[1][1].children()
                canon_task = args[0][1].expr.name
                ml_task    = args[1][1].expr.name
                add_task(ml_task, canon_task)
                print("  TASK_OVERRIDE", canon_task, ml_task)

            else:
                if type(node.children()[0][1]) == c_ast.ArrayDecl:
                    if type(node.children()[0][1].children()[0][1]) == c_ast.TypeDecl:
                        if type(node.children()[0][1].children()[0][1].children()[0][1]) == c_ast.Struct:
                            struct_name = node.children()[0][1].children()[0][1].children()[0][1].name
                            if struct_name == "menu_entry":
                                if len(node.children()) == 2:
                                    entries = node.children()[1][1]
                                    parse_menu_entries(entries)

class FuncDefVisitor(c_ast.NodeVisitor):
    def visit_FuncDef(self, node):
        func_name = node.decl.name
        if "static" in node.decl.storage:
            func_name = current_file + ":" + func_name
        
        is_func[func_name] = True
        func_list.append((func_name, node.decl.coord.file, node.decl.coord.line))
        func_storage[func_name] = node.decl.storage

        if func_name.startswith("_prop_handler_"):
            add_task(func_name, "PropMgr")
            print("  PROP_HANDLER", func_name[14:])

        if func_name.startswith("__module_prophandler_"):
            add_task(func_name, "PropMgr")
            print("  PROP_HANDLER", func_name[21:])

class FuncCallVisitor(c_ast.NodeVisitor):
    def visit_FuncCall(self, node):

        # task_create function calls
        if node.name.name == "task_create":
            if type(node.args.exprs[3]) == c_ast.ID:
                func_name = node.args.exprs[3].name
                task_name = node.args.exprs[0].value.strip('"')
                add_task(func_name, task_name)
                print("  task_create", task_name, func_name)
            else:
                print("  task_create at %s skipped" % node.name.coord)
            return

        # delayed_call's run from timer interrupts
        if node.name.name == "delayed_call":
            add_task(node.args.exprs[1].name, "timer interrupt")
            return

        # process functions called from another one's arguments
        # e.g. printf("%f", sqrt(x))
        if node.args:
            for n in node.args.exprs:
                if type(n) == c_ast.FuncCall:
                    add_func_call(n.name)

                if type(n) == c_ast.Cast:
                    try: decl = n.children()[0][1].children()[0][1].children()[0][1]
                    except: decl = None
                    if type(decl) == c_ast.FuncDecl:
                        func = n.children()[1][1]
                        print(node.name.name, "(...", n.children()[1][1].name, "...)")
                        add_func_call(func)

                if type(n) == c_ast.ID:
                    if is_func[n.name]:
                        add_func_call(n)

        if type(node.name) == c_ast.ID:
            add_func_call(node.name)
        else:
            print("  dynamic call at %s skipped: %s" % (node.name.coord, type(node.name)))

def scan_structs(ast):
    v = StructVisitor()
    v.visit(ast)

def scan_func_calls(ast):
    v = FuncCallVisitor()
    v.visit(ast)

def scan_func_defs(ast):
    v = FuncDefVisitor()
    v.visit(ast)

def get_tasks(func, level=0):
    #~ print(" " * level, func)
    tasks = deepcopy(func_tasks[func])

    # assume non-static functions can be called from anywhere
    if 0:
        if ("static" not in func_storage[func] and
           not func.startswith("_prop_handler_")):
            tasks.append("ANY");

    # check all callers recursively
    for caller in set(func_calls[func]):
        if caller != func:
            tasks += get_tasks(caller, level+1)

    return list(set(tasks))

# tag each function with the task(s) where it's called from,
# with autogenerated comments before the function body: /*** ... ***/
def tag_functions(source_file, dest_file):
    lines = open(source_file).readlines()
    i = 0
    with open(dest_file, "w") as out:
        for f,s,l in sorted(func_list, key=lambda x: x[1]):
            if s != source_file:
                continue
            while i < l-2:
                out.write(lines[i])
                i += 1
            
            keywords_before = lines[i].strip().split(" ")
            if (";" in lines[i] or
               not (set(["static", "int", "void"]) & set(keywords_before))):
                out.write(lines[i])
                i += 1
            
            if lines[i-1].strip() != "" and not lines[i-1].strip().startswith("/"):
                out.write("\n")
            
            if func_tasks[f]:
                func_tasks[f] = list(set(func_tasks[f]))
                task_cmt = ", ".join(func_tasks[f])
                out.write("/*** %s ***/\n" % task_cmt);
            
            if 1:
                tasks = get_tasks(f);
                if tasks != func_tasks[f] or not func_tasks[f]:
                    task_cmt = \
                        "ANY task" if "ANY" in tasks else (
                        ", ".join(tasks) if tasks else "?")
                    out.write("/*** %s ***/\n" % task_cmt);

                # couldn't find it? try printing the callers
                if not tasks:
                    callers = set(func_calls[f])
                    callers = ["%s (%s)" % (friendly_func_name(f), ",".join(get_tasks(f))) for f in callers]
                    if callers:
                        out.write("/*** Called from %s ***/\n" % ", ".join(callers));


        while i < len(lines):
            out.write(lines[i])
            i += 1

# remove these autogenerated comments: /*** ... ***/
def cleanup_tags(source_file, dest_file):
    lines = open(source_file).readlines()
    with open(dest_file, "w") as out:
        for l in lines:
            if l.startswith("/*** ") and l.endswith(" ***/\n"):
                continue
            out.write(l)

if __name__ == "__main__":
    for current_file in source_files + module_files:
        print(current_file)
        cleanup_tags(current_file, current_file)
        if current_file in source_files:
            ast = parse_src(current_file)
        else:
            ast = parse_module(current_file)
        scan_func_defs(ast)
        scan_structs(ast)
        scan_func_calls(ast)

    # overwrite files (we have them in hg anyway)
    for current_file in source_files + module_files:
        print("Writing %s..." % current_file)
        tag_functions(current_file, current_file)

    print("Finished.")

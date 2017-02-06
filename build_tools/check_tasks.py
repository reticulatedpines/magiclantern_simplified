# Tag each ML function with the task(s) where it's called from.
#
# Uses static analysis (pycparser) and identifies common idioms
# from ML code (TASK_CREATE, PROP_HANDLER, menu declarations etc).
#
# Should work with both Python 2 and 3.

from __future__ import print_function
import os, sys, re
from collections import defaultdict
from pycparser import c_parser, c_ast, preprocess_file
from copy import copy, deepcopy

def lookup_c(filepath):
    # to be called from platform/cam
    # (to match .i files)
    if filepath.startswith("modules/"):
        dir, file = os.path.split(filepath)
        name, ext = os.path.splitext(file)
        cfile = os.path.join(dir, name + ".c")
        assert os.path.isfile(cfile)
        return cfile

    dir, file = os.path.split(filepath)
    name, ext = os.path.splitext(file)
    cfile = os.path.join(dir, name + ".c")
    if os.path.isfile(cfile): return cfile
    cfile = os.path.join("../../src", name + ".c")
    if os.path.isfile(cfile): return cfile
    
    print(filepath, dir, file, cfile)
    assert 0

prepro_files = [f for f in sys.argv if f.endswith(".i")]
source_files = [lookup_c(f) for f in prepro_files]

def parse_prepro(filename):
    os.system("sed -i 's/asm volatile/asm/' '%s'" % filename)
    os.system("sed -i 's/asm __volatile__/asm/' '%s'" % filename)

    cpp_path = "arm-none-eabi-cpp"
    cpp_args = [
        '-Dasm(...)=',
        '-D__asm__(...)=',
        '-D__extension__=',
        '-D__attribute__(...)=',
        '-D__builtin_va_list=int',
        '-D__builtin_va_arg(a,b)=0',
    ]

    text = preprocess_file(filename, cpp_path, cpp_args)
    
    # pycparser doesn't like a C file to begin with ;
    # (happens after removing the first asm block from arm-mcr.h)
    text = "struct __dummy__;\n" + text;
    #~ print(text)

    return c_parser.CParser().parse(text, lookup_c(filename))

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
    func_name = func_name.replace("_prop_handler_", "PROP_HANDLER:")
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
    lines_raw = open(source_file).readlines()

    # filter old autogenerated comments
    last = -1
    line_map = []
    lines = []
    for l in lines_raw:
        if l.startswith("/*** ") and l.endswith(" ***/\n"):
            pass
        else:
            last += 1
            lines.append(l)
        line_map.append(last)
    lines.append("")

    i = 0
    with open(dest_file, "w") as out:
        for f,s,l in sorted(func_list, key=lambda x: x[1]):
            if s != source_file:
                continue
            while i < line_map[l-1]-1:
                out.write(lines[i])
                i += 1
            
            if "PROP_INT" in lines[i]:
                continue
            
            keywords_before = lines[i].strip().split(" ")
            if (";" in lines[i] or
               not (set(["static", "int", "void"]) & set(keywords_before))):
                out.write(lines[i])
                i += 1
            
            if lines[i-1].strip() != "" and not lines[i-1].strip().startswith("/"):
                out.write("\n")

            # if the function name is not present on the next line, show it in comments
            # (happens if the function was defined in some macro, see e.g. cfn.c)
            func_short = f.split(":")[-1]
            if func_short.startswith("_prop_handler_") or func_short.startswith("__module_prophandler_"):
                func_short = "PROP_HANDLER"
            show_func = (func_short not in lines[i] + lines[i+1])

            if func_tasks[f]:
                func_tasks[f] = list(set(func_tasks[f]))
                task_cmt = ", ".join(func_tasks[f])
                if show_func:
                    task_cmt = "%s: %s" % (friendly_func_name(f), task_cmt)
                out.write("/*** %s ***/\n" % task_cmt);
            
            if 1:
                tasks = get_tasks(f);
                if tasks != func_tasks[f] or not func_tasks[f]:
                    task_cmt = \
                        "ANY task" if "ANY" in tasks else (
                        ", ".join(tasks) if tasks else "?")
                    if show_func:
                        task_cmt = "%s: %s" % (friendly_func_name(f), task_cmt)
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
    
    if "clean" in sys.argv:
        for current_file in source_files:
            print("cleaning", current_file)
            cleanup_tags(current_file, current_file)
        raise SystemExit
    
    start_dir = os.getcwd()
    for prepro_file in prepro_files:
        if prepro_file.startswith("../../modules/"):
            module_path, prepro_file = os.path.split(prepro_file)
            print("Entering", module_path)
            os.chdir(module_path)
        else:
            os.chdir(start_dir)
        current_file = lookup_c(prepro_file)
        print(current_file, prepro_file)
        ast = parse_prepro(prepro_file)
        scan_func_defs(ast)
        scan_structs(ast)
        scan_func_calls(ast)

    # overwrite files (we have them in hg anyway)
    for current_file in source_files:

        if current_file.startswith("../../modules/"):
            module_path, current_file = os.path.split(current_file)
            print("Entering", module_path)
            os.chdir(module_path)
        else:
            os.chdir(start_dir)

        print("Writing %s..." % current_file)
        tag_functions(current_file, current_file)

    print("Finished.")

#!/usr/bin/env python3

import os
import sys
import tempfile
import subprocess


class ModuleError(Exception):
    pass


class Module:
    def __init__(self, name):
        # We expect to be run in the modules dir,
        # so the name is also the name of a subdir.
        #
        # E.g. dot_tune, and we expect these related
        # files to exist:
        # modules/dot_tune/dot_tune.mo
        # modules/dot_tune/dot_tune.dep
        # modules/dot_tune/dot_tune.sym
        self.mo_file = os.path.join(name, name + ".mo")
        self.dep_file = os.path.join(name, name + ".dep")
        self.sym_file = os.path.join(name, name + ".sym")
        self.name = name
        #self.required_mods = None # deliberately not set, so we can easily
                                   # distinguish between empty and not-initialised

        # get required symbols
        with open(self.dep_file, "r") as f:
            self.deps = {d.rstrip() for d in f}
        self.unsatisfied_deps = self.deps

        # get exported_symbols (often empty),
        # lines are of format:
        # 0x1f0120 some_name
        with open(self.sym_file, "r") as f:
            self.syms = {s.strip().split()[1] for s in f}

    def __str__(self):
        s = "Module: %s\n" % self.name
        s += "\t%s\n" % self.mo_file
        s += "\tUnsat deps:\n"
        for d in self.unsatisfied_deps:
            s += "\t\t%s\n" % d
        s += "\tSyms:\n"
        for sym in self.syms:
            s += "\t\t%s\n" % sym
        return s

    def add_cross_module_deps(self, modules):
        """
        Compares the exports of the given list of modules, to the
        required dependencies of this module.  If matches are found,
        record that in self.required_modules.

        This can be used at runtime to automatically load required
        modules.
        """
        non_self_mods = [m for m in modules if m.name != self.name]
        module_exports = {} # dict of "exportName:moduleName"
        for m in non_self_mods:
            for s in m.syms:
                module_exports[s] = m.name

        required_mods = {module_exports[d] for d in self.deps if d in module_exports}
        self.required_mods = required_mods

    def add_module_dep_section(self, objcopy):
        """
        Alters module's file on disk, to record self.required_mods
        into .module_deps section.

        objcopy should be path to an objcopy executable valid for
        the binary format of module files; ARM ELF.

        If required_mods is empty, the section is not added.  See
        add_cross_module_deps().
        """
        try:
            if self.required_mods:
                pass
            else:
                return
        except AttributeError:
            return

        mod_name_data = b""
        with tempfile.NamedTemporaryFile() as dep_file:
            for d in self.required_mods:
                mod_name_data += d.encode("utf8") + b"\0"
            mod_name_data += b"\0" # 0 length string to mark end of array
            dep_file.write(mod_name_data)
            dep_file.flush()
            objcopy_invoke = [objcopy,
                              "--add-section", ".module_deps=" + dep_file.name,
                              self.mo_file]
            # this might fail on Windows, because it is dumb and doesn't like
            # opening the same file twice
            if mod_name_data:
                print("writing .module_deps section for module: %s" % self.name)
                subprocess.run(objcopy_invoke)
            # objcopy is also kind of dumb.  You can't use add-section if the section
            # already exists, you can't use update-section if it doesn't.
            # You get a rather unhelpful error like this if you run add-section twice:
            # arm-none-eabi-objcopy: dual_iso/stTJMQwP: can't add section '.module_deps': file format not recognized

            # Currently, I can't be bothered fixing this, so, each cam will try to
            # add-section and most likely fail because the modules didn't need rebuilding,
            # so the section exists from last time.  This is okay.
            # Ideally we'd only run this step if modules had changed.

        #print("mod name data: %s" % mod_name_data)


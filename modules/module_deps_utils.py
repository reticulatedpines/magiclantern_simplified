#!/usr/bin/env python3

import os
import sys


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


if __name__ == "__main__":
    main()

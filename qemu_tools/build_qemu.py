#!/usr/bin/env python3

import os
import argparse
import subprocess
from collections import namedtuple

def main():
    args = parse_args()

    starting_dir = os.getcwd()
#    check_deps() # maybe?  Could just list them

    try:
        obtain_gcc()
        build_qemu(args.qemu_source_dir, args.qemu_output_dir)
        obtain_gdb()
    except QemuInstallerError as e:
        print("ERROR: " + str(e))
        os.chdir(starting_dir)
        exit(-1)

    os.chdir(starting_dir)


class QemuInstallerError(Exception):
    pass


Version = namedtuple("Version", "major minor micro")

def get_qemu_version(source_dir):
    version_filepath = os.path.join(source_dir, "VERSION")
    if not os.path.isfile(version_filepath):
        raise QemuInstallerError("Missing VERSION file")

    with open(version_filepath, "r") as f:
        version_string = f.readline()

    version_parts = version_string.split(".")
    return Version(int(version_parts[0]),
                   int(version_parts[1]),
                   int(version_parts[2]))


def build_qemu(source_dir, output_dir):
    version = get_qemu_version(source_dir)
    # not actually using version yet, but it
    # will be useful for choosing which gcc to use,
    # what deps to use etc.  Might want to move this
    # check higher up.

    os.chdir(source_dir)
    try:
        res = subprocess.run(["./configure", "--target-list=arm-softmmu",
                              "--disable-docs", "--enable-vnc", "--enable-gtk"],
                             env=os.environ, check=True)
    except subprocess.CalledProcessError as e:
        raise QemuInstallerError("qemu configure failed: " + str(e))
    
    cpus = str(len(os.sched_getaffinity(0)))
    try:
        res = subprocess.run(["make", "-j" + cpus],
                             env=os.environ, check=True)
    except subprocess.CalledProcessError as e:
        raise QemuInstallerError("qemu make failed: " + str(e))


def obtain_gcc():
    pass


def obtain_gdb():
    pass


def parse_args():
    description = """
    Script to build Qemu with EOS support, and associated tools
    (principally GDB, some versions are buggy around setting
    dynamic breakpoints in ARM)
    """

    script_dir = os.path.split(os.path.realpath(__file__))[0]
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("-s", "--qemu_source_dir",
                        default=os.path.realpath(os.path.join(script_dir,
                                                              "..", "..", "qemu")),
                        help="source dir for ML Qemu, default: %(default)s")

    parser.add_argument("-q", "--qemu_output_dir",
                        default=os.path.realpath(os.path.join(script_dir,
                                                              "..", "..", "qemu-eos")),
                        help="output dir for Qemu, default: %(default)s")
    # FIXME check if dir already exists and handle correctly

    args = parser.parse_args()

    try:
        if not os.path.isdir(args.qemu_source_dir):
            raise QemuInstallerError("Qemu source dir didn't exist.  "
                                     "You may need to clone the qemu-eos repo.")
        if not os.path.isdir(os.path.join(args.qemu_source_dir,
                                          ".git")):
            raise QemuInstallerError("Qemu source dir didn't look like a git repo.  "
                                     "It should contain the qemu-eos repo.")
    except QemuInstallerError as e:
        print("ERROR: " + str(e))
        exit(-1)

    return args


if __name__ == "__main__":
    main()

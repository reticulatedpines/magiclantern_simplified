#!/usr/bin/env python3

import os
import argparse
import subprocess

def main():
    args = parse_args()

    starting_dir = os.getcwd()
    try:
        build_qemu(args.qemu_source_dir, args.qemu_output_dir)
    except QemuInstallerError as e:
        print("ERROR: " + str(e))
        os.chdir(starting_dir)
        exit(-1)


class QemuInstallerError(Exception):
    pass


def build_qemu(source_dir, output_dir):
    script_dir = os.path.split(os.path.realpath(__file__))[0]
    target_tar = os.path.join(script_dir, "docker_builder", "qemu_src.tar")
    try:
        res = subprocess.run(["./scripts/archive-source.sh", target_tar],
                             env=os.environ, check=True,
                             cwd=source_dir)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise QemuInstallerError("Tarring qemu source failed: " + str(e))

    # we could pass this to docker?
    #cpus = str(len(os.sched_getaffinity(0)))

    try:
        res = subprocess.run(["sudo", "docker", "build", "-t", "qemu_build",
                              "docker_builder"],
                             universal_newlines=True,
                             env=os.environ, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise QemuInstallerError("sudo docker build failed: " + str(e))

    try:
        res = subprocess.run(["sudo", "docker", "rm", "qemu_build_output"],
                             universal_newlines=True,
                             env=os.environ)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise QemuInstallerError("sudo docker rm failed: " + str(e))

    try:
        res = subprocess.run(["sudo", "docker", "create", "--name", "qemu_build_output",
                              "qemu_build"],
                             universal_newlines=True,
                             env=os.environ)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise QemuInstallerError("sudo docker create failed: " + str(e))

    try:
        res = subprocess.run(["sudo", "docker", "cp", "qemu_build_output:/home/ml_builder/qemu.zip",
                              "."],
                             universal_newlines=True,
                             env=os.environ)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise QemuInstallerError("sudo docker cp failed: " + str(e))


def parse_args():
    description = """
    Script to build Qemu with EOS support, using Docker.
    The Qemu binaries are created as qemu.zip.
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

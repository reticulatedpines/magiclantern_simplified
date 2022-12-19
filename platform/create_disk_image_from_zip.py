#!/usr/bin/env python3

import os
import sys
import argparse
import subprocess
import shutil
import zipfile
import lzma
import hashlib
from time import sleep


def main():
    args = parse_args()

    qemu_mount_dir = "qemu_disk_mount"
    if os.path.isdir(qemu_mount_dir):
        # rmdir will fail if still mounted, this is good
        # as it indicates some problem we want to be aware of
        os.rmdir(qemu_mount_dir)
    os.mkdir(qemu_mount_dir)

    qcow_name = "sd.qcow2"
    cf_name = "cf.qcow2"
    pid = 0

    # delete old disk images to ensure builds either give you
    # a current image, or nothing.
    if os.path.isfile(cf_name):
        os.remove(cf_name)
    if os.path.isfile(qcow_name):
        os.remove(qcow_name)
    if os.path.isfile(qcow_name + ".xz"):
        os.remove(qcow_name + ".xz")

    # decompress the base image
    with open(qcow_name, "wb") as outf:
        qcow_data = lzma.open(os.path.join("..", "sd.qcow2.xz")).read()
        outf.write(qcow_data)

    # copy the new build into the image filesystem
    try:
        # guestmount requires libguestfs-tools, and won't work by default on Ubuntu
        # because they're stupid: https://bugs.launchpad.net/ubuntu/+source/linux/+bug/759725
        pid_file = "guestmount.pid"
        subprocess.run(["guestmount", "--pid-file", pid_file,
                        "-a", qcow_name,
                        "-m", "/dev/sda1", qemu_mount_dir],
                        check=True)

        with open(pid_file, "r") as f:
            pid = int(str(f.read()))

        old_autoexec = os.path.join(qemu_mount_dir, "autoexec.bin")
        if os.path.isfile(old_autoexec):
            os.remove(old_autoexec)

        old_ml = os.path.join(qemu_mount_dir, "ML")
        if os.path.isdir(old_ml):
            shutil.rmtree(old_ml, ignore_errors=True)

        with zipfile.ZipFile(args.build_zip, 'r') as z:
            z.extractall(qemu_mount_dir)
    finally:
        subprocess.run(["guestunmount", qemu_mount_dir])
        os.rmdir(qemu_mount_dir)

    # Extraordinarily annoying, but libguestfs doesn't sync changes
    # to the image file before guestunmount exits.  There's not even
    # an option to do this.
    #
    # The manpage tells us we must wait for the guestmount pid
    # we got earlier to exit.  How stupid.
    if pid:
        sleep(1) # on my system this is reliably enough time to wait
        if is_pid(pid):
            print("Waiting for guestmount to exit...")
        timeout = 15
        while is_pid(pid) and timeout:
            sleep(1)
            timeout -= 1
        if timeout == 0:
            # I've never observed this so don't know what mangled state
            # we might be in.  Maybe we should cleanup image files?
            print("Guestmount failed to exit, failing!")
            sys.exit(-1)
    else:
        # we should raise earlier if guestmount errored and never get here,
        # but just in case:
        print("No pid obtained from guestmount.  Shouldn't be possible?")
        sys.exit(-1)
    if os.path.isfile(pid_file):
        os.remove(pid_file)

    # clone to the CF image if things went okay
    if os.path.isfile(qcow_name):
        shutil.copyfile(qcow_name, cf_name)

    # If we got the guestmount waiting logic wrong earlier,
    # these files can differ!  Final sanity check:
    with open(qcow_name, "rb") as f:
        sd_hash = hashlib.sha1(f.read())
    with open(cf_name, "rb") as f:
        cf_hash = hashlib.sha1(f.read())
    if sd_hash.digest() != cf_hash.digest():
        print("CF and SD qcow2 images didn't match hashes, failing!")
        sys.exit(-1)


def is_pid(pid):        
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def parse_args():
    description = """
    Given a path to a magiclantern platform build zip, e.g.
    magiclantern-Nightly.blah.200D.zip, use guestmount to
    create a qemu disk image with the contents, ready for emulation.

    Expected only to be called by the build system, via 'make disk_image'
    """
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("build_zip",
                        help="Path to ML build zip, required")

    args = parser.parse_args()

    if not os.path.isfile(args.build_zip):
        print("Couldn't access build zip")
        sys.exit(-1)

    return args


if __name__ == "__main__":
    main()

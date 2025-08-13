#!/usr/bin/env python3

import os
import argparse
import sys
import subprocess
import shutil


def main():
    args = parse_args()
    if shutil.which("git"):
        status = subprocess.run(["git", "status"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if "not a git repository" in status.stdout.decode("utf8"):
            print("unofficial: not a git repo", end='')
        else:
            short_hash = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).strip().decode("utf8")
            branch = subprocess.check_output(["git", "branch", "--show-current"]).strip().decode("utf8")
            print(short_hash + " " + branch, end='')
    else:
        print("unofficial: no git", end='')


def parse_args():
    description = """
    Used during ML build to get git summary information,
    for use in version.c.  ML can display this info to the user.
    Similar info to "hg id".
    """

    parser = argparse.ArgumentParser(description=description)
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    main()

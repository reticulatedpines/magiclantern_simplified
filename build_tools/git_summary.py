#!/usr/bin/env python3

import os
import argparse
import sys
import subprocess


def main():
    args = parse_args()
    short_hash = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).strip().decode("utf8")
    branch = subprocess.check_output(["git", "branch", "--show-current"]).strip().decode("utf8")
    print(short_hash + " " + branch, end='')


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

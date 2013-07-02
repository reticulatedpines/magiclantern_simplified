#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os

def system_or_exit(cmdline):
    ret = os.system(cmdline)
    if ret != 0:
        sys.exit(1)

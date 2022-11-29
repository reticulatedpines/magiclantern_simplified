import os, platform, subprocess

def check_command(shell):
    return subprocess.check_call(["which", shell])

def check_all():    
    print("Checking for rst2html")
    if check_command("rst2html"):
        print("rst2html not found")
        print("Install with: pip3 install rst2html")
        exit(1)
    else:
        print("rst2html checked")

    print("Checking for python2")
    if check_command("python2"):
        print("python2 not found. ML still relies on it.")
        exit(1)
    else:
        print("python2 works")

    print("Checking for host GCC")
    if check_command("gcc"):
        print("gcc not found")
        exit(1)
    else:
        print("gcc checked")

    print("Checking for ARM GCC")
    if check_command("arm-none-eabi-gcc"):
        print("Could not find arm-none-eabi-gcc")
        exit(1)

check_all()

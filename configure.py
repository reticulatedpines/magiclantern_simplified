import os, platform, subprocess

def checkCommand(shell):
    if platform.system() == "Windows":
        return os.system("where " + shell)
    else:
        return os.system("which " + shell)

def checkAll():
    print("Checking for valid bash")
    if checkCommand("bash"):
        print("bash not found")
        print("You will want a POSIX compatible environment")
        exit(1)
    else:
        print("bash checked")
    
    print("Checking for rst2html")
    if checkCommand("rst2html"):
        print("rst2html not found")
        print("Install with: pip3 install rst2html")
        exit(1)
    else:
        print("rst2html checked")

    print("Checking for python2")
    if checkCommand("python2"):
        print("python2 not found. ML still relies on it.")
        exit(1)
    else:
        print("python2 works")

    print("Checking for pip2")
    if checkCommand("pip2"):
        print("pip2 not found. ML still relies on it.")
        exit(1)
    else:
        print("python2 works")
        py1 = subprocess.check_output(["python2", "--version"])
        py2 = subprocess.check_output(["python", "--version"])
        if py1 == py2:
            print("python is python2")
        else:
            print("You will need to route python to python2, you can install 'python-is-python2'")
    print("Checking for host GCC")
    if checkCommand("gcc"):
        print("gcc not found")
        exit(1)
    else:
        print("gcc checked")

    print("Checking for ARM GCC")
    if checkCommand("arm-none-eabi-gcc"):
        print("Could not find arm-none-eabi-gcc")
        exit(1)
    else:
        version = subprocess.check_output(["arm-none-eabi-gcc", "--version"])
        if "20160919" in str(version):
            print("Compiler is correct version for ML")
        else:
            print("This version of ARM GCC isn't fully supported, you may run into issues compiling")

checkAll()

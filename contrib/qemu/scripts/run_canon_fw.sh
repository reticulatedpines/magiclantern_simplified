#!/usr/bin/env bash

# exit codes:
# 0 = emulation ran
# 1 = SD or CF image mounted
# 2 = compiling QEMU failed

GREP=${GREP:=grep}
QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
MAKE=${MAKE:=make}

if [ $(uname) == "Darwin" ]; then
    if [[ -n $(which ggrep) ]]; then
        GREP=ggrep
    else
        echo
        echo "Error: you need GNU grep to run this script"
        echo "brew install grep"
        exit 1
    fi
fi

# better way to check whether a disk image is mounted?
# or, how to tell QEMU to use exclusive access for the disk images?
function is_mounted
{
    # try lsof first
    # check whether QEMU is started with or without -snapshot
    # accept -snapshot and --snapshot, but not ---snapshot or -snapshots or -other-snapshot
    if [[ " ${BASH_ARGV[*]} " =~ .*\ --?snapshot\ .* ]]; then
        # started with -snapshot
        # run if other processes have the SD/CF image file opened as read-only
        # fail if other processes have the SD/CF image file opened with write access
        # http://unix.stackexchange.com/a/115722
        if lsof +c 0 "$1" 2>/dev/null | awk '$4~/[0-9]+[uw -]/' | $GREP -F "$1"; then
            return 0
        fi
    else
        # started without -snapshot
        # fail if other processes have the SD/CF image file opened, no matter what kind of access
        if lsof +c 0 "$1" 2>/dev/null | $GREP -F "$1"; then
            return 0
        fi
    fi

    if [ $(uname) == "Darwin" ]; then
        # on Mac, lsof is enough
        # further checks are Linux-only anyway
        return 1
    fi

    # now find out whether the image file is mounted
    # lsof doesn't seem to cover this case, why?

    # use losetup if available
    if command -v losetup; then
        # this finds out whether the image file is mounted and where
        SD_DEV=`losetup -j "$1" | $GREP -Po "(?<=/dev/)[^ ]*(?=:)"`
        if [ $? == 0 ]; then
            # this may return multiple matches; try them all
            for sd_dev in $SD_DEV; do
                if cat /proc/mounts | $GREP /dev/$sd_dev; then
                    return 0
                fi
                if cat /proc/mounts | $GREP /dev/mapper/$sd_dev; then
                    return 0
                fi
            done
        fi
    else
        # try to guess from mount output (approximate)
        if mount | $GREP -F "$(realpath $1)"; then
            # this matches images mounted manually with:
            # mount -o loop,offset=... sd.img /mount/point
            return 0
        fi
        if mount | $GREP -o /dev/mapper/loop.*EOS_DIGITAL; then
            # this matches kpartx mounts, but can't tell whether SD or CF
            # or maybe some other EOS_DIGITAL image is mounted
            echo "Might be a different image, please check."
            return 0
        fi
    fi

    return 1
}

if is_mounted sd.img; then
    echo
    echo "Error: please unmount the SD image."
    exit 1
fi

if is_mounted cf.img; then
    echo
    echo "Error: please unmount the CF image."
    exit 1
fi

# recompile QEMU
$MAKE -C $QEMU_PATH || exit 2

if [ -t 1 ] ; then
    # clear the terminal (only if running in interactive mode, not when redirected to logs)
    # (since the logs are very large, being able to scroll at the beginning is helpful)
    # note: "tput reset" may crash when running as a background job, figure out why
    printf '\ec\e[3J'
fi

# print the invocation
# https://unix.stackexchange.com/a/118468
case $(ps -o stat= -p $$) in
  *+*) echo $0 "$@" ;;      # Running in foreground
  *) echo $0 "$@" "&" ;;    # Running in background
esac

# also print the command-line of arm-none-eabi-gdb / gdb-multiarch, if any
# note: some systems limit process names to 15 chars
gdb_pid=$(pgrep -P $PPID -n arm-none-eabi-g || pgrep -P $PPID -n gdb-multiarch)
if [ "$gdb_pid" != "" ]; then
  gdb_cmd=$(ps -p $gdb_pid -o args | tail -n1)
  case $(ps -o stat= -p $gdb_pid) in
    *+*) echo "$gdb_cmd" ;;      # Running in foreground
    *) echo "$gdb_cmd" "&" ;;    # Running in background
  esac
fi

echo

CAM=${1//,*/}
if [ "$CAM" ] && [ ! "$QEMU_EOS_DEBUGMSG" ]; then
    QEMU_EOS_DEBUGMSG=`cat $CAM/debugmsg.gdb | $GREP DebugMsg_log -B 1 | $GREP -Pom1 "(?<=b \*)0x.*"`
    echo "DebugMsg=$QEMU_EOS_DEBUGMSG (from GDB script)"
else
    echo "DebugMsg=$QEMU_EOS_DEBUGMSG (overriden)"
fi

# Mac: bring QEMU window to foreground
# fixme: easier way?
# fixme: doesn't work with multiple instances
if [ -t 1 ] && [ $(uname) == "Darwin" ]; then
    ( sleep 0.5; osascript -e 'tell application "System Events" to tell process "qemu-system-arm" to set frontmost to true' &>/dev/null ) &
fi

# run the emulation
env QEMU_EOS_DEBUGMSG="$QEMU_EOS_DEBUGMSG" \
  $QEMU_PATH/arm-softmmu/qemu-system-arm \
    -drive if=sd,format=raw,file=sd.img \
    -drive if=ide,format=raw,file=cf.img \
    -chardev socket,server,nowait,path=qemu.monitor$QEMU_JOB_ID,id=monsock \
    -mon chardev=monsock,mode=readline \
    -name $CAM \
    -M $*

# note: QEMU monitor is redirected to Unix socket qemu.monitor
# so you can interact with the emulator with e.g. netcat:
#
#    echo "log io" | nc -U qemu.monitor
#
# or, for interactive monitor console:
#
#    socat - UNIX-CONNECT:qemu.monitor
#
# you can, of course, redirect it with -monitor stdio or -monitor vl
# more info: http://nairobi-embedded.org/qemu_monitor_console.html

# QEMU_JOB_ID should not generally be defined (just leave it blank)
# exception: if you want to launch multiple instances of the emulator,
# each instance will get its own qemu.monitor socket.
#
# If you start multiple instances, also use -snapshot to prevent changes
# to the SD and CF card images (so they can be shared between all processes)

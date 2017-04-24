#!/bin/bash

# script options (environment variables)
# example: TIMEOUT=20 AUTOEXEC_ONLY=1 ./run_ml_all_cams.sh

TIMEOUT=${TIMEOUT:=10}                      # timeout for default QEMU monitor script
QEMU_ARGS=${QEMU_ARGS:=}                    # command-line arguments for QEMU
QEMU_SCRIPT=${QEMU_SCRIPT:=sleep $TIMEOUT}  # QEMU monitor script (echo quit is always appended)
LOG_PREFIX=${LOG_PREFIX:=}                  # prefix for log files
LOG_SUFFIX=${LOG_SUFFIX:=}                  # suffix for log files
GDB_SCRIPT=${GDB_SCRIPT:=patches.gdb}       # GDB script (skipped if not found)
BOOT=${BOOT:=1}                             # whether to load autoexec.bin (default: load)
INCREMENTAL=${INCREMENTAL:=}                # skip make clean (default: full rebuild)
AUTOEXEC_ONLY=${AUTOEXEC_ONLY:=}            # copy only autoexec.bin (default: make zip and full install)
ML_OPTIONS=${ML_OPTIONS:=}                  # ML compile options (e.g. "CONFIG_QEMU=y")

QEMU_SCRIPT="$QEMU_SCRIPT; echo quit"

. ./mtools_setup.sh

cd ../magic-lantern/platform
for cam in [[:upper:]]*/ [[:digit:]]*/; do 
    # only compile ML if BOOT=1
    if [ "$BOOT" == "1" ]; then

        # make clean (optional)
        if [ ! "$INCREMENTAL" ]; then
           make -C $cam clean
        fi

        # compile ML, skip this target if there are any errors
        make -C $cam $ML_OPTIONS || continue

        # go to QEMU dir and copy ML to the card images
        if [ "$AUTOEXEC_ONLY" ]; then
            cd ../../qemu/
            mcopy -o -i $MSD ../magic-lantern/platform/$cam/autoexec.bin ::
            mcopy -o -i $MCF ../magic-lantern/platform/$cam/autoexec.bin ::
        else
            make -C $cam zip VERSION=qemu $ML_OPTIONS
            rm -rf qemu-tmp/
            mkdir qemu-tmp
            unzip $cam/magiclantern-qemu.zip -d qemu-tmp -q
            cd ../../qemu/
            mcopy -o -i $MSD ../magic-lantern/platform/$cam/* ::
            mcopy -o -i $MCF ../magic-lantern/platform/$cam/* ::
            mcopy -o -s -i $MSD ../magic-lantern/platform/qemu-tmp/ML/ ::
            mcopy -o -s -i $MCF ../magic-lantern/platform/qemu-tmp/ML/ ::
            rm -rf qemu-tmp/
        fi
    fi

    # get cam name (e.g. 50D) and cam name with firmware version (e.g. 50D.111)
    CAM=${cam//.*/}
    CAM_FW=${cam////}

    # setup QEMU command line
    LOGNAME=$LOG_PREFIX$CAM_FW$LOG_SUFFIX.log
    QEMU_INVOKE="./run_canon_fw.sh $CAM,firmware='boot=$BOOT' -display none -monitor stdio $QEMU_ARGS"

    if [ "$GDB_SCRIPT" ] && [ -f "$CAM/$GDB_SCRIPT" ]; then
        # GDB runs unattended (from script)
        # while QEMU listens to monitor commands from stdin
        QEMU_INVOKE="( 
      arm-none-eabi-gdb -x $CAM/$GDB_SCRIPT & 
      $QEMU_INVOKE -s -S
    )
    "
    fi

    # print QEMU command line
    export CAM
    export CAM_FW
    echo
    echo "$QEMU_SCRIPT \\" | envsubst
    echo "  | $QEMU_INVOKE &> $LOGNAME" | envsubst
    echo

    # invoke QEMU
    (eval $QEMU_SCRIPT) \
        | eval $QEMU_INVOKE &> $LOGNAME

    cd ../magic-lantern/platform/
done

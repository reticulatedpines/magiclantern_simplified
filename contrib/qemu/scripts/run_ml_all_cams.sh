#!/bin/bash

# script options (environment variables)
# example: TIMEOUT=10 AUTOEXEC_ONLY=1 ./run_ml_all_cams.sh

TIMEOUT=${TIMEOUT:=20}                      # timeout for default QEMU monitor script
SCREENSHOT=${SCREENSHOT:=}                  # optional screenshot ($CAM_FW.ppm)
QEMU_ARGS=${QEMU_ARGS:=}                    # command-line arguments for QEMU
QEMU_SCRIPT=${QEMU_SCRIPT:=sleep $TIMEOUT}  # QEMU monitor script (echo quit is always appended, optionally screenshot too)
LOG_PREFIX=${LOG_PREFIX:=}                  # prefix for log files
LOG_SUFFIX=${LOG_SUFFIX:=}                  # suffix for log files
GDB_SCRIPT=${GDB_SCRIPT:=patches.gdb}       # GDB script (skipped if not found)
BOOT=${BOOT:=1}                             # whether to load autoexec.bin (default: load)
INCREMENTAL=${INCREMENTAL:=}                # skip make clean (default: full rebuild)
AUTOEXEC_ONLY=${AUTOEXEC_ONLY:=}            # copy only autoexec.bin (default: make zip and full install)
ML_OPTIONS=${ML_OPTIONS:=}                  # ML compile options (e.g. "CONFIG_QEMU=y")
BUILD_DIR=${BUILD_DIR:=platform/\$CAM_DIR}  # optionally build a different target; usually requires AUTOEXEC_ONLY=1
                                            # e.g. "minimal/\$CAM_DIR", "minimal/qemu-frsp" (with ML_OPTIONS="MODEL=\$CAM"), "installer/\$CAM_DIR"

[ "$SCREENSHOT" ] && QEMU_SCRIPT="$QEMU_SCRIPT; echo screendump \$CAM_FW.ppm"
QEMU_SCRIPT="$QEMU_SCRIPT; echo quit"

. ./mtools_setup.sh

cd ../magic-lantern/platform
for CAM_DIR in [[:upper:]]*/ [[:digit:]]*/; do 
    # CAM_DIR is e.g. 50D.111/ (includes a slash)
    # get cam name (e.g. 50D) and cam name with firmware version (e.g. 50D.111)
    CAM=${CAM_DIR//.*/}
    CAM_FW=${CAM_DIR////}
    FW=${CAM_FW//*./}

    # only specify firmware version to QEMU for 5D3
    [ "$CAM" = "5D3" ] && QFW="$FW;" || QFW=

    # replace camera-specific variables in script arguments 
    export CAM
    export FW
    export CAM_FW
    export CAM_DIR
    BuildDir=`echo $BUILD_DIR | envsubst`
    QemuArgs=`echo $QEMU_ARGS | envsubst`
    QemuScript=`echo $QEMU_SCRIPT | envsubst`
    MLOptions=`echo $ML_OPTIONS | envsubst`

    # only compile ML if BOOT=1
    if [ "$BOOT" == "1" ]; then

        # make clean (optional)
        if [ ! "$INCREMENTAL" ]; then
            echo make -C ../$BuildDir clean $MLOptions
            make -C ../$BuildDir clean $MLOptions
        fi

        # compile ML, skip this target if there are any errors
        make -C ../$BuildDir $MLOptions || continue

        # go to QEMU dir and copy ML to the card images
        if [ "$AUTOEXEC_ONLY" ]; then
            cd ../../qemu/
            mcopy -o -i $MSD ../magic-lantern/$BuildDir/autoexec.bin ::
            mcopy -o -i $MCF ../magic-lantern/$BuildDir/autoexec.bin ::
        else
            make -C $CAM_DIR zip VERSION=qemu $MLOptions
            rm -rf qemu-tmp/
            mkdir qemu-tmp
            unzip -q $CAM_DIR/magiclantern-qemu.zip -d qemu-tmp
            cd ../../qemu/
            mcopy -o -i $MSD ../magic-lantern/platform/$CAM_DIR/* ::
            mcopy -o -i $MCF ../magic-lantern/platform/$CAM_DIR/* ::
            mcopy -o -s -i $MSD ../magic-lantern/platform/qemu-tmp/ML/ ::
            mcopy -o -s -i $MCF ../magic-lantern/platform/qemu-tmp/ML/ ::
            rm -rf qemu-tmp/
        fi

        # export any ML symbols we might want to use in QEMU
        . ./export_ml_syms.sh $CAM_FW
    fi

    # setup QEMU command line
    LogName=$LOG_PREFIX$CAM_FW$LOG_SUFFIX.log
    QemuInvoke="./run_canon_fw.sh $CAM,firmware='${QFW}boot=$BOOT' -display none -monitor stdio $QemuArgs"

    if [ "$GDB_SCRIPT" ] && [ -f "$CAM/$GDB_SCRIPT" ]; then
        # GDB runs unattended (from script)
        # while QEMU listens to monitor commands from stdin
        QemuInvoke="( 
      arm-none-eabi-gdb -x $CAM/$GDB_SCRIPT & 
      $QemuInvoke -s -S
    )
    "
    fi

    # print QEMU command line
    echo
    echo "( $QemuScript )\\"
    echo "  | $QemuInvoke &> $LogName"
    echo

    # invoke QEMU
    (eval $QemuScript) \
        | eval $QemuInvoke &> $LogName

    cd ../magic-lantern/platform/
done

#!/usr/bin/env bash

# script options (environment variables)
# example: TIMEOUT=10 AUTOEXEC_ONLY=1 ./run_ml_all_cams.sh

ML_PATH=${ML_PATH:=../magic-lantern}
QEMU_DIR=${QEMU_DIR:=qemu-eos}              # QEMU working directory (same level as the magic-lantern directory)
TIMEOUT=${TIMEOUT:=20}                      # timeout for default QEMU monitor script
SCREENSHOT=${SCREENSHOT:=}                  # optional screenshot ($CAM_FW.ppm)
QEMU_ARGS=${QEMU_ARGS:=}                    # command-line arguments for QEMU
QEMU_SCRIPT=${QEMU_SCRIPT:=sleep $TIMEOUT}  # QEMU monitor script (echo quit is always appended, optionally screenshot too)
LOG_PREFIX=${LOG_PREFIX:=}                  # prefix for log files (e.g. foo-5D3.123.log)
LOG_SUFFIX=${LOG_SUFFIX:=}                  # suffix for log files (e.g. 5D3.123-bar.log)
LOG_NAME=${LOG_NAME:=\
    '$LOG_PREFIX$CAM_FW$LOG_SUFFIX.log'}    # custom log file name (e.g. '$CAM/baz.log')
GDB_SCRIPT=${GDB_SCRIPT:=patches.gdb}       # GDB script (skipped if not found)
BOOT=${BOOT:=1}                             # whether to load autoexec.bin (default: load)
INCREMENTAL=${INCREMENTAL:=}                # skip make clean (default: full rebuild)
AUTOEXEC_ONLY=${AUTOEXEC_ONLY:=}            # copy only autoexec.bin (default: make zip and full install)
BUILD_DIR=${BUILD_DIR:='platform/$CAM_DIR'} # optionally build a different target; usually requires AUTOEXEC_ONLY=1
                                            # e.g. 'minimal/$CAM_DIR', "minimal/qemu-frsp" (with ML_OPTIONS='MODEL=$CAM'), 'installer/$CAM_DIR'
ML_PLATFORMS=${ML_PLATFORMS:=\
    "[[:upper:]]*.*/ [[:digit:]]*.*/"}      # specify ML platforms to run, e.g. "5D3.123/ 700D.114/"
ML_CHANGESET=${ML_CHANGESET:=}              # specify which HG changeset to compile and run (hg up -C)
ML_OPTIONS=${ML_OPTIONS:=}                  # ML compile options (e.g. "CONFIG_QEMU=y")

[ "$SCREENSHOT" ] && QEMU_SCRIPT="$QEMU_SCRIPT
echo screendump \$CAM_FW.ppm"

QEMU_SCRIPT="$QEMU_SCRIPT
echo quit"

. ./mtools_setup.sh

cd $ML_PATH/platform

for CAM_DIR in $ML_PLATFORMS; do 
    # CAM_DIR is e.g. 50D.111/ (includes a slash)
    # get cam name (e.g. 50D) and cam name with firmware version (e.g. 50D.111)
    CAM=${CAM_DIR//.*/}
    CAM_FW=${CAM_DIR////}
    FW=${CAM_FW//*./}

    echo
    echo "Emulating $CAM $FW..."
    echo "====================="
    echo 

    # only specify firmware version to QEMU for 5D3
    [ "$CAM" == "5D3" ] && QFW="$FW;" || QFW=

    # replace camera-specific variables in script arguments 
    export CAM
    export FW
    export CAM_FW
    export CAM_DIR
    BuildDir=`echo "$BUILD_DIR" | envsubst`
    QemuArgs=`echo "$QEMU_ARGS" | envsubst`
    QemuScript=`echo "$QEMU_SCRIPT" | envsubst`
    MLOptions=`echo "$ML_OPTIONS" | envsubst`
    LogName=`echo "$LOG_NAME" | envsubst`

    # only compile ML if BOOT=1
    if [ "$BOOT" == "1" ]; then

        # use a specific changeset (optional)
        if [ "$ML_CHANGESET" ]; then
            cd ../$BuildDir
            echo hg update $ML_CHANGESET --clean
            hg update $ML_CHANGESET --clean
            cd -
        fi

        # make clean (optional)
        if [ ! "$INCREMENTAL" ]; then
            echo make -C ../$BuildDir clean $MLOptions
            make -C ../$BuildDir clean $MLOptions
        fi

        # compile ML, skip this target if there are any errors
        make -C ../$BuildDir $MLOptions || continue

        # go to QEMU dir and copy ML to the card images
        if [ "$AUTOEXEC_ONLY" ]; then
            cd ../../$QEMU_DIR
            mcopy -o -i $MSD $ML_PATH/$BuildDir/autoexec.bin ::
            mcopy -o -i $MCF $ML_PATH/$BuildDir/autoexec.bin ::
        else
            make -C ../$BuildDir install_qemu $MLOptions
            cd ../../$QEMU_DIR
        fi

        # export any ML symbols we might want to use in QEMU
        . ./export_ml_syms.sh $BuildDir
    else
        # back to QEMU directory without compiling
        cd ../../$QEMU_DIR

        # clear previously-exported symbols, if any
        . ./export_ml_syms.sh clear
    fi

    # setup QEMU command line
    QemuInvoke="./run_canon_fw.sh $CAM,firmware='${QFW}boot=$BOOT' \\
    -display none -monitor stdio $QemuArgs"

    if [ "$GDB_SCRIPT" ] && [ -f "$CAM/$GDB_SCRIPT" ]; then
        # GDB runs unattended (from script)
        # while QEMU listens to monitor commands from stdin
        QemuInvoke="arm-none-eabi-gdb -x $CAM/$GDB_SCRIPT & 
$QemuInvoke -s -S"
    fi

    # print QEMU command line
    echo
    echo "( "
    echo "$QemuScript" | sed 's/^/  /'
    echo ") | ("
    echo "$QemuInvoke" | sed 's/^/  /'
    echo ") &> $LogName"
    echo

    # invoke QEMU
    ( eval "$QemuScript" ) \
        | ( eval "$QemuInvoke" ) &> $LogName

    cd $ML_PATH/platform/
done

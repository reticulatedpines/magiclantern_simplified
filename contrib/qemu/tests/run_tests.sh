#!/usr/bin/env bash

# Emulator tests
# This also shows the emulation state on various cameras
# usage: 
#   ./run_test.sh                   # test all models
#   ./run_test.sh 5D3 EOSM EOSM3    # test only specific models (uppercase arguments = camera models)
#   ./run_test.sh 5D3 drysh menu    # run only the selected tests/models (lowercase arguments = test names)

# Caveat: this assumes no other qemu-system-arm or
# arm-none-eabi-gdb processes are running during the tests

EOS_CAMS=( 5D 5D2 5D3 5D4 6D 7D 7D2M
           40D 50D 60D 70D 80D
           400D 450D 500D 550D 600D 650D 700D 750D 760D
           100D 1000D 1100D 1200D 1300D EOSM EOSM2 )

POWERSHOT_CAMS=( EOSM3 EOSM10 EOSM5 A1100 )

EOS_SECONDARY_CORES=( 5D3eeko 5D4AE 7D2S )

# cameras able to run Canon GUI (menu tests)
GUI_CAMS=( 5D2 5D3 50D 60D 70D
           450D 500D 550D 600D 650D 700D
           100D 1000D 1100D 1200D EOSM EOSM2 )

# cameras with a SD card
SD_CAMS=( 5D3 5D4 6D 60D 70D 80D
          450D 500D 550D 600D 650D 700D 750D 760D
          100D 1000D 1100D 1200D 1300D EOSM EOSM2 )

# cameras with a CF card
CF_CAMS=( 5D 5D2 5D3 5D4 7D 7D2M 40D 50D 400D )

# cameras able to run the FA_CaptureTestImage test (full-res silent picture backend)
FRSP_CAMS=( 5D3 500D 550D 50D 60D 1100D 1200D )

# newer openbsd netcat requires -N (since 1.111)
# older openbsd netcat does not have -N (prints error if we attempt to use it)
# try to autodetect which one should be used, and let the user override it
NC=${NC:=$(nc -h |& grep -q -- -N && echo "nc -N" || echo "nc")}
echo "Using netcat: $NC"

function has_upper_args {
    for arg in "$@"; do
        if [ "$arg" == "${arg^^}" ]; then
            return 0
        fi
    done
    return 1
}

function has_lower_args {
    for arg in "$@"; do
        if [ "$arg" == "${arg,,}" ]; then
            return 0
        fi
    done
    return 1
}

if (( $# > 0 )) && has_upper_args "$@"; then
    # uppercase arguments present? test only these models
    # fixme: nicer way to do the same? (intersection between arguments and the lists of supported models)
    REQ_CAMS=( $* )
    EOS_CAMS=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${EOS_CAMS[@]}" | sort -u) | sort -n))
    POWERSHOT_CAMS=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${POWERSHOT_CAMS[@]}" | sort -u) | sort -n))
    GUI_CAMS=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${GUI_CAMS[@]}" | sort -u) | sort -n))
    SD_CAMS=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${SD_CAMS[@]}" | sort -u) | sort -n))
    CF_CAMS=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${CF_CAMS[@]}" | sort -u) | sort -n))
    FRSP_CAMS=($(join <(printf %s\\n "${FRSP_CAMS[@]}" | sort -u) <(printf %s\\n "${FRSP_CAMS[@]}" | sort -u) | sort -n))
    EOS_SECONDARY_CORES=($(join <(printf %s\\n "${REQ_CAMS[@]}" | sort -u) <(printf %s\\n "${EOS_SECONDARY_CORES[@]}" | sort -u) | sort -n))
fi

# lowercase args are test names
function test_selected {
    local test=$1
    shift
    if has_lower_args ${BASH_ARGV[@]}; then
        # lowercase args present?
        for arg in ${BASH_ARGV[@]}; do
            if [ "$arg" == "$test" ]; then
                # current test selected
                return 0
            fi
        done

        # this test is not among the lowercase args? skip it
        return 1
    fi

    # no lowercase args? run all tests
    return 0
}

declare -A MENU_SEQUENCE
MENU_SEQUENCE[5D2]="f1 i i i m up up up space m m w w p p" # sensor cleaning animation
MENU_SEQUENCE[5D3]="f1 i i i f1 i m left down down down space m m p p q space m right right space m down right space pgdn m q"
MENU_SEQUENCE[50D]="f1 i i i m up space space m w w p p" # sensor cleaning animation
MENU_SEQUENCE[60D]="f1 i i i i m left left up space m m p p"
#MENU_SEQUENCE[70D]="m up up up up space m up up" # fixme: locks up quickly
MENU_SEQUENCE[70D]="f1" # fixme: doesn't go into menu
MENU_SEQUENCE[450D]="f1 m up up space m left left i p p w down down space " # LiveView overlays also working, but the tests would crash at shutdown
MENU_SEQUENCE[500D]="f1 m i i right right up m p p"
MENU_SEQUENCE[550D]="m i i right right down down down space space p p" # info screen not working
MENU_SEQUENCE[600D]="i i m right right right right up space down down space m p p q q" # starts with sensor cleaning animation; no info screen unless we enable it
MENU_SEQUENCE[650D]="f1 m right right p p" # starts in movie mode, no lens
MENU_SEQUENCE[700D]="f1 m right down space right space p p" # starts in movie mode, no lens
MENU_SEQUENCE[100D]="f1 i i i m left left left up up left left left left left left left up up space up space p p"
MENU_SEQUENCE[1000D]="f1 i w w i p p m space space up up space m left i"
MENU_SEQUENCE[1100D]="f1 i i m i i left m p p down right space right right space up right space" # drive mode not working
MENU_SEQUENCE[1200D]="f1 i i m i i space m m p p down right space right right space up right space" # drive mode not working
MENU_SEQUENCE[EOSM]="f1 m up up up space m up space m up space m left down down down space space p p " # only menu works
MENU_SEQUENCE[EOSM2]="f1 m space space space up up space m up space m up space m up space m right space space m m" # only menu works

FMT_SEQ="space right space wait f1 space"
FMT_SEQ_5D3="space space right space wait f1 space space"
# these are customized for my ROM dumps (keys required to select the Format menu)
# TODO: some generic way to navigate to Format menu?
declare -A FORMAT_SEQUENCE
FORMAT_SEQUENCE[5D2]="m right right right right $FMT_SEQ"
FORMAT_SEQUENCE[5D3]="m left left left left $FMT_SEQ_5D3"
FORMAT_SEQUENCE[50D]="m down down $FMT_SEQ"
FORMAT_SEQUENCE[60D]="m left left left left $FMT_SEQ"
FORMAT_SEQUENCE[450D]="m left left $FMT_SEQ"                # fixme: locks up
FORMAT_SEQUENCE[500D]="m $FMT_SEQ"
FORMAT_SEQUENCE[550D]="m $FMT_SEQ"
FORMAT_SEQUENCE[600D]="m right right right $FMT_SEQ"
FORMAT_SEQUENCE[650D]="m $FMT_SEQ"                          # fixme: free space wrong before format
FORMAT_SEQUENCE[700D]="m left left left $FMT_SEQ"           # fixme: free space wrong before format
FORMAT_SEQUENCE[100D]="m $FMT_SEQ"                          # fixme: free space wrong before format
FORMAT_SEQUENCE[1000D]="m left left $FMT_SEQ"               # fixme: locks up
FORMAT_SEQUENCE[1100D]="m right right down $FMT_SEQ"
FORMAT_SEQUENCE[1200D]="m left left $FMT_SEQ"
FORMAT_SEQUENCE[EOSM]="m left left left $FMT_SEQ"
FORMAT_SEQUENCE[EOSM2]="m left left left up $FMT_SEQ"

function set_gui_timeout {
    if [ $CAM == "100D" ]; then
        # 100D appears slower, for some reason
        GUI_TIMEOUT=10
    else
        # 500D needs less than 2 seconds; let's be a bit conservative
        GUI_TIMEOUT=5
    fi
}


# We will use mtools to alter and check the SD/CF image contents.
# fixme: hardcoded partition offset
MSD=sd.img@@50688
MCF=cf.img@@50688

# mtools doesn't like our SD image, for some reason
export MTOOLS_SKIP_CHECK=1
export MTOOLS_NO_VFAT=1



# this script runs from qemu/tests/ so we have to go up one level
cd ..

echo "Compiling..."
./run_canon_fw.sh help &> build.log

# don't recompile each time (for speed)
export MAKE="echo skipping make"

# Most of the tests require custom SD/CF card imags.
# The one that comes with our QEMU install script is perfect.
echo
echo "Setting up temporary SD/CF card images..."

function sd_restore {
  trap '' SIGINT
  echo
  echo "Restoring your SD/CF card images..."
  mv sd-user.img sd.img
  mv cf-user.img cf.img
  trap - SIGINT
}

# disable CTRL-C while moving the files
trap '' SIGINT
mv sd.img sd-user.img
mv cf.img cf-user.img
trap sd_restore EXIT
trap - SIGINT

cp -v ../magic-lantern/contrib/qemu/sd.img.xz .
unxz -k sd.img.xz
cp sd.img cf.img

# checksum of reference SD/CF images
sd_checksum=`md5sum sd.img`
cf_checksum=`md5sum cf.img`

# optional argument: -q = quiet
function sd_cf_restore_if_modified {
    # compute the checksums of SD and CF images in parallel
    local checksums="$( md5sum sd.img & md5sum cf.img & )"
    sd_new_checksum=$(echo "$checksums" | grep sd.img)
    cf_new_checksum=$(echo "$checksums" | grep cf.img)

    if [ "$sd_new_checksum" != "$sd_checksum" ]; then
        [ "$1" == "-q" ] || echo -n " SD modified "
        rm sd.img
        unxz -k sd.img.xz
    fi
    if [ "$cf_new_checksum" != "$cf_checksum" ]; then
        [ "$1" == "-q" ] || echo -n " CF modified "
        rm cf.img
        cp sd.img cf.img
    fi
}

# send a keypress and wait for the screen to match a checksum
# save the screenshot, regardless of match result
# vncexpect key md5 timeout capture
function vncexpect {
    vncdo -s :12345 key $1
    rm -f $4
    if vncdo -s :12345 -v -v -t $3 expect-md5 $2 &> tests/vncdo.log; then
        echo -n "."
        vncdo -s :12345 capture $4
        return 0
    else
        vncdo -s :12345 capture $4
        echo ""
        echo "$2  expected"
        md5sum $4
        return 1
    fi
    return $ret
}

function kill_qemu {
    # fixme: only kill the QEMU processes started by us (how?)

    if [ "$1" == "expect_running" ] && ! pidof qemu-system-arm > /dev/null; then
        echo -e "\e[31mQEMU not running\e[0m"
    fi

    if [ "$1" == "expect_not_running" ] && pidof qemu-system-arm > /dev/null; then
        echo -e "\e[31mshutdown error\e[0m (QEMU still running)"
    fi

    if killall -TERM -w qemu-system-arm 2>/dev/null; then
        sleep 1
    fi

    if pidof arm-none-eabi-gdb > /dev/null; then
        echo -e "\e[31mGDB still running\e[0m"

        # for some reason, -TERM may hang up here
        # but should be unreachable normally
        killall -9 -w arm-none-eabi-gdb 2>/dev/null
    fi
}

# just to be sure
kill_qemu expect_not_running

# generic helper for running tests
# arguments: 
# - test function name (without the test_ prefix)
# - CAM (camera name)
# - test name override (optional; derived from test function name by default)
function run_test {

    # global variables used in tests
    TEST=${3:-${1//_/-}}
    CAM=$2

    if ! test_selected $TEST; then
        # can we skip this test?
        return
    fi

    printf "%7s: " $CAM

    # make sure the test subdirectory is present
    # and remove any previous logs or screenshots from this test
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/$TEST*.log
    rm -f tests/$CAM/$TEST*.png
    rm -f tests/$CAM/$TEST*.ppm

    # run the test function
    test_$1
}

# called after running each set of tests
function cleanup {
    kill_qemu expect_not_running
    sd_cf_restore_if_modified -q
}

# All EOS cameras should run the Dry-shell console over UART
# fixme: only works on D4 and D5 models
function test_drysh {

    # skip VxWorks models for now
    if grep -q VxWorks $CAM/ROM1.BIN; then
        echo -e "\e[33mskipping\e[0m"
        return
    fi

    # same for DIGIC 6
    if grep -q ZicoAssert $CAM/ROM1.BIN; then
        echo -e "\e[33mskipping\e[0m"
        return
    fi

    # most models require "akashimorino" to enable the Dry-shell
    # a few don't (500D, 5D3eeko), but sending it anyway shouldn't hurt

    (
        sleep 5; echo "akashimorino";
        sleep 0.5; echo "drysh";
        sleep 0.5; echo "vers";
        sleep 0.5; echo "?";
        sleep 0.5; echo "task";
        sleep 0.5; echo "quit" | $NC -U qemu.monitor;
        sleep 0.5;
    ) | (
        if [ -f $CAM/patches.gdb ]; then
            ( arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit 1>&2 &
              ./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot \
                -display none -serial stdio -s -S ) \
                    > tests/$CAM/$TEST.log \
                    2> tests/$CAM/$TEST-emu.log
        else
            ./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot \
                -display none -serial stdio \
                > tests/$CAM/$TEST.log \
                2> tests/$CAM/$TEST-emu.log
        fi
    )

    kill_qemu

    tests/check_grep.sh tests/$CAM/$TEST.log \
        -oEm1 "Dry-shell .*\..*| xd .*" || return

    tests/check_grep.sh tests/$CAM/$TEST.log \
        -q "akashimorino"
}

echo
echo "Testing Dry-shell over UART..."
for CAM in 5D3eeko ${EOS_CAMS[*]}; do
    run_test drysh $CAM
done; cleanup


# Interrupts are generally non-deterministic; however, we can use
# the -icount option in QEMU to get a deterministic execution trace.
# We'll let the emulation run until no new functions are discovered
# (that is, until the autogenerated IDC script no longer grows).

# Eeko is also a good target for testing call/return trace, because:
# - simple Thumb code from the bootloader (unlike DIGIC 6 cameras,
#   where the bootloader is plain ARM)
# - the overall structure is different enough to cause issues

function test_calls_main {

    # log all function calls/returns and export to IDC
    # log task switches as well
    # use -icount to get a deterministic execution
    # fixme: execution under gdb is not deterministic, even with -icount
    # ansi2txt used here (only once) because it's very slow
    ./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -icount 5 \
        -display none -d calls,idc,tasks \
        -serial file:tests/$CAM/$TEST-uart.log \
        |& ansi2txt > tests/$CAM/$TEST-raw.log &

    # wait for uart log to appear
    while [ ! -f tests/$CAM/$TEST-uart.log ]; do sleep 1; done
    sleep 1

    # check for boot message
    if grep -qE "([KR].* (READY|AECU)|Dry|Boot)" tests/$CAM/$TEST-uart.log; then
      msg=`grep --text -oEm1 "([KR].* (READY|AECU)|Dry|Boot)[a-zA-Z >]*" tests/$CAM/$TEST-uart.log`
      printf "%-16s" "$msg"
    else
      echo -en "\e[33mBad output\e[0m      "
    fi
    echo -n ' '

    # wait until it the IDC no longer grows, up to 5 minutes
    ./wait_log.sh $CAM.idc 300 5 -q "^}" 
    echo
    echo -n "                          "

    sleep 1
    kill_qemu expect_running

    # as the emulation is not stopped exactly at the same time,
    # we have to trim it somewhere in order to get repeatable results.
    # in FROMUTILITY it was easy - after some point, the log contents
    # was repeating over and over, so remove_end_loop.py was doing a fine job.
    # in the main firmware, things are not always repeating with a simple pattern.
    # stopping when IDC no longer grows is not reliable either - it depends a lot on the PC speed
    # let's trim until matching the MD5 of $TEST-basic.idc
    # this assumes the needles (expected test results) were created on a slower PC and/or using a smaller timeout

    cat $CAM.idc | grep -o "MakeFunction(.*)" \
        > tests/$CAM/$TEST-basic.idc

    # compute the MD5 of $TEST-basic.idc, line by line, until it matches its reference MD5
    # man, this python3 is quite painful with all these unicode errors...
    cd tests/$CAM/
    cat $TEST.md5 | grep $TEST-basic.idc | python3 -c 'if True:
        import os, sys
        from hashlib import md5
        ref_md5, filename = sys.stdin.readline().split()
        m = md5()
        trimmed = b""
        with open(filename, "rb") as input:
            for l in input.readlines():
                trimmed += l
                m.update(l)
                if m.hexdigest() == ref_md5:
                    break
        if trimmed:
            with open(filename, "wb") as output:
                output.write(trimmed)
    '
    cd $OLDPWD

    # trim the log file after the last function call identified in the IDC
    last_call=`tail -1 tests/$CAM/$TEST-basic.idc | grep -om1 "0x[^,]*"`
    last_call_thumb=`printf "0x%X\n" $((last_call+1))`

    # extract call/return lines, task switches and interrupts
    cat tests/$CAM/$TEST-raw.log \
        | grep -E "call |return |Task switch |interrupt " \
        | sed -n "1,/call $last_call\|call $last_call_thumb/ p" \
        > tests/$CAM/$TEST.log

    # extract only the basic info (call address indented, return address)
    # useful for checking when the log format changes
    # fixme: how to transform "return 0xVALUE to 0xADDR" into "return to 0xADDR" with shell scripting?
    cat tests/$CAM/$TEST.log | grep -oP " *call 0x[0-9A-F]+| to 0x[0-9A-F]+" \
        > tests/$CAM/$TEST-basic.log

    # also copy the IDC file for checking its MD5
    # this works on CF models too, even if some nondeterminism is present
    # the IDC needs trimming, too, as it doesn't always stop at the same line
    cat $CAM.idc | sed -n "1,/MakeFunction($last_call/ p" > tests/$CAM/$TEST.idc
    cat $CAM.idc | tail -n 2 >> tests/$CAM/$TEST.idc

    # extract only the call address from IDC
    # useful for checking when some additional info changes (e.g. comments)
    cat tests/$CAM/$TEST.idc | grep -o "MakeFunction(.*)" \
        > tests/$CAM/$TEST-basic.idc

    # count some stats
    calls=`cat tests/$CAM/$TEST.log | grep -c "call "`
    retns=`cat tests/$CAM/$TEST.log | grep "return " | grep -vc "interrupt"`
    ints=`cat tests/$CAM/$TEST.log | grep -E "interrupt .*at " | grep -vc "return"`
    reti=`cat tests/$CAM/$TEST.log | grep -c "return from interrupt"`
    nints=`cat tests/$CAM/$TEST.log | grep -E " interrupt .*at " | grep -vc "return"`
    nreti=`cat tests/$CAM/$TEST.log | grep -c " return from interrupt"`
    tsksw=`cat tests/$CAM/$TEST.log | grep -c "Task switch to "`
    tasks=`cat tests/$CAM/$TEST.log | grep -oP "(?<=Task switch to )[^:]*" | sort | uniq | head -3 |  tr '\n' ' '`
    if (( ints == 0 )); then
      echo -en "\e[33mno interrupts\e[0m "
    else
      echo -n "$ints ints"
      if (( reti == 0 )); then
        echo -e ", \e[31mno reti\e[0m"
        return
      fi
      if (( nints != 0 )); then echo -n " ($nints nested)"; fi
      echo -n ", $reti reti"
      if (( nreti != 0 )); then echo -n " ($nreti nested)"; fi
      if (( ints - nints > reti + 1 )); then
          echo -en " \e[33mtoo few reti\e[0m "
      fi
      if (( reti > ints )); then
          echo -e " \e[31mtoo many reti\e[0m"
          return
      fi
      if (( tsksw > 0 )); then
         echo -n ", $tsksw task switches ( $tasks)"
      else
         echo -en ", \e[33mno task switches\e[0m"
      fi
    fi

    # if we are checking only the IDC, say so
    echo
    echo -n "                          "
    grep -q '.log' tests/$CAM/$TEST.md5 \
        && echo -n "$calls calls, $retns returns " \
        || echo -n "IDC "

    tests/check_md5.sh tests/$CAM/ $TEST || cat tests/$CAM/$TEST.md5.log
}

echo
echo "Testing call/return trace on main firmware..."
for CAM in ${EOS_SECONDARY_CORES[*]} ${EOS_CAMS[*]}; do
    run_test calls_main $CAM
done; cleanup


# We should get a valid call/return trace on the bootloader,
# which loads FROMUTILITY if autoexec.bin is not present on a bootable card.
# There are no timed interrupts here, so the process should be deterministic.
# On SD models, this process is deterministic without any trickery.
# For some reason, the CF emulation is not deterministic even with -icount,
# so we'll only check the IDC for these models (todo: figure out why).
# The results are assumed to be correct, but they were not thoroughly checked.
# Feel free to report bugs, corner cases and so on.

function test_calls_from {

    # log all function calls/returns and export to IDC
    ./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot \
        -display none -d calls,idc \
        -serial file:tests/$CAM/$TEST-uart.log \
        &> tests/$CAM/$TEST-raw.log &

    # wait until the FROMUTILITY menu appears
    touch tests/$CAM/$TEST-uart.log
    (timeout 20 tail -f -n100000 tests/$CAM/$TEST-uart.log & ) \
        | grep -q "FROMUTIL"
    sleep 0.5

    kill_qemu expect_running

    # extract call/return lines
    # remove infinite loop at the end, if any
    ansi2txt < tests/$CAM/$TEST-raw.log \
        | grep -E "call |return " \
        | python remove_end_loop.py \
        > tests/$CAM/$TEST.log

    # count some stats
    calls=`cat tests/$CAM/$TEST.log | grep -c "call "`
    retns=`cat tests/$CAM/$TEST.log | grep "return " | grep -vc "interrupt"`

    # extract only the basic info (call address indented, return address)
    # useful for checking when the log format changes
    # fixme: how to transform "return 0xVALUE to 0xADDR" into "return to 0xADDR" with shell scripting?
    cat tests/$CAM/$TEST.log | grep -oP " *call 0x[0-9A-F]+| to 0x[0-9A-F]+" \
        > tests/$CAM/$TEST-basic.log

    # also copy the IDC file for checking its MD5
    # this works on CF models too, even if some nondeterminism is present
    cp $CAM.idc tests/$CAM/$TEST.idc

    # extract only the call address from IDC
    # useful for checking when some additional info changes (e.g. comments)
    cat tests/$CAM/$TEST.idc | grep -o "MakeFunction(.*)" \
        > tests/$CAM/$TEST-basic.idc

    if grep -q "FROMUTIL" tests/$CAM/$TEST-uart.log; then
        grep -oEm1 "FROMUTIL[^*]*" tests/$CAM/$TEST-uart.log | tr -d '\n'
    elif grep -q "AUTOEXEC.BIN not found" tests/$CAM/$TEST-uart.log; then
        echo -en "\e[33mFROMUTILITY not executed  \e[0m"
    else
        echo -e "\e[31mFAILED!\e[0m"
        return
    fi

    # if we are checking only the IDC, say so
    grep -q '.log' tests/$CAM/$TEST.md5 \
        && echo -n "$calls calls, $retns returns " \
        || echo -n "IDC "

    tests/check_md5.sh tests/$CAM/ $TEST || cat tests/$CAM/$TEST.md5.log
}

echo
echo "Testing call/return trace on fromutility..."

# remove autoexec.bin from card images (to get the FROMUTILITY menu)
mdel -i $MSD ::/autoexec.bin
mdel -i $MCF ::/autoexec.bin

# run the tests
for CAM in ${EOS_CAMS[*]}; do
    run_test calls_from $CAM
done; cleanup


# At each point, the verbose call stack should match the call/return trace
# This feature is also exercised in the context of interrupts and DryOS task switches

function test_calls_cstack {

    # skip VxWorks models for now
    if grep -q VxWorks $CAM/ROM1.BIN; then
        echo "skipping"
        return
    fi

    # log all function calls/returns, interrupts
    # and DebugMsg calls with call stack for each message
    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot \
             -display none -d calls,tasks,debugmsg,v -s -S \
             -serial file:tests/$CAM/$TEST-uart.log & \
           arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &> tests/$CAM/$TEST-raw.log
    else
        ./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot \
            -display none -d calls,tasks,debugmsg,v \
            -serial file:tests/$CAM/calls-cstack-uart.log \
            &> tests/$CAM/$TEST-raw.log &
    fi
    sleep 10
    kill_qemu expect_running

    ansi2txt < tests/$CAM/$TEST-raw.log | python tests/test_callstack.py &> tests/$CAM/$TEST-test.log \
        && (tail -n 1 tests/$CAM/$TEST-test.log | tr -d '\n'; echo " OK" ) \
        || (tail -n 1 tests/$CAM/$TEST-test.log | tr -d '\n'; echo -e " \e[31mFAILED!\e[0m" )
}

echo
echo "Testing callstack consistency with call/return trace for DebugMsg calls..."
for CAM in ${EOS_CAMS[*]}; do
    run_test calls_cstack $CAM
done; cleanup


# -d callstack triggered quite a few nondeterministic assertions during development
# so, running it on all cameras where Canon menu can be navigated should be a good test
# logging the entire call/return trace is possible, but very slow and not repeatable
function test_menu_callstack {

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 -d callstack -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &> tests/$CAM/$TEST.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 -d callstack &) \
            &> tests/$CAM/$TEST.log
    fi

    set_gui_timeout
    sleep $(( 2*GUI_TIMEOUT ))

    count=0;
    for key in ${MENU_SEQUENCE[$CAM]}; do
        # some GUI operations are very slow under -d callstack (many small functions called)
        # for most of them, 1 second is enough, but the logic would be more complex
        vncdotool -s :12345 key $key; sleep 3
        vncdotool -s :12345 capture tests/$CAM/$TEST$((count++)).png
        echo -n .
    done

    kill_qemu expect_running

    tests/check_md5.sh tests/$CAM/ $TEST || cat tests/$CAM/$TEST.md5.log
}

echo
echo "Testing Canon menu with callstack enabled..."
for CAM in ${GUI_CAMS[*]}; do
    run_test menu_callstack $CAM menu
done; cleanup


# All EOS cameras should emulate the bootloader
# and jump to main firmware.
# Also list blocks copied to RAM during startup, if any.
function test_boot {

    # sorry, couldn't get the monitor working together with log redirection...
    (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot \
        -display none \
        -serial file:tests/$CAM/$TEST-uart.log \
        -d romcpy,int \
        &> tests/$CAM/$TEST.log \
    ) &

    # wait until some READY-like message is printed on the UART
    touch tests/$CAM/$TEST-uart.log
    ( timeout 2 tail -f -n100000 tests/$CAM/$TEST-uart.log & ) \
        | grep -qE "([KR].* (READY|AECU).*|Intercom.*|Dry>)"

    kill_qemu expect_running

    # fixme: duplicate regex
    tests/check_grep.sh tests/$CAM/$TEST-uart.log \
        -oE "([KR].* (READY|AECU).*|Intercom.*|Dry>)"

    # print ROMCPY messages before the first interrupt
    ansi2txt < tests/$CAM/$TEST.log \
        | grep -m1 -B 100000 -E "Taking exception|terminating on signal" \
        | grep -oE "\[ROMCPY\].*" \
        | sed -e "s/\[ROMCPY\]/        /"
}

echo
echo "Testing bootloaders..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]}; do
    run_test boot $CAM
done; cleanup


# All EOS cameras should load autoexec.bin, run HPTimer functions
# and print current task name (this actually tests some ML stubs)
function test_hptimer {

    # compile it from ML dir, for each camera
    HPTIMER_PATH=../magic-lantern/minimal/qemu-hptimer
    rm -f $HPTIMER_PATH/autoexec.bin
    make MODEL=$CAM -C $HPTIMER_PATH clean &>> tests/$CAM/$TEST-build.log
    make MODEL=$CAM -C $HPTIMER_PATH       &>> tests/$CAM/$TEST-build.log
    
    if [ ! -f $HPTIMER_PATH/autoexec.bin ]; then
        echo -e "\e[31mCompile error\e[0m"
        return
    fi

    # copy autoexec.bin to card images
    mcopy -o -i $MSD $HPTIMER_PATH/autoexec.bin ::
    mcopy -o -i $MCF $HPTIMER_PATH/autoexec.bin ::

    # run the HPTimer test
    ./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot -display none &> tests/$CAM/$TEST.log &

    # wait for He (Hello) from qprintf (blue, each char colored) 
    touch tests/$CAM/$TEST.log
    ( timeout 1 tail -f -n100000 tests/$CAM/$TEST.log & ) | grep --binary-files=text -qP "\x1B\x5B34mH\x1B\x5B0m\x1B\x5B34me\x1B\x5B0m"

    # let it run for 1 second
    sleep 1
    kill_qemu expect_running
    
    tests/check_grep.sh tests/$CAM/$TEST.log -m1 "Hello from task run_test"
    printf "       "
    tests/check_grep.sh tests/$CAM/$TEST.log -m1 "Hello from HPTimer" && return
    printf "       "
    tests/check_grep.sh tests/$CAM/$TEST.log -m1 "Hello from task init" && return
}

echo
echo "Testing HPTimer and task name..."
for CAM in ${EOS_CAMS[*]}; do
    run_test hptimer $CAM
done; cleanup


# All cameras booting the GUI should be able to navigate Canon menu:
function test_menu {

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 -d debugmsg -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &> tests/$CAM/$TEST.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 -d debugmsg &) \
            &> tests/$CAM/$TEST.log
    fi

    set_gui_timeout
    sleep $GUI_TIMEOUT

    # note: these should also work via qemu.monitor
    # (slightly different keycodes and only PPM screenshots available)
    count=0;
    for key in ${MENU_SEQUENCE[$CAM]}; do
        vncdotool -s :12345 key $key; sleep 0.5
        vncdotool -s :12345 capture tests/$CAM/$TEST$((count++)).png
        echo -n .
    done

    # shutdown event
    echo "system_powerdown" | $NC -U qemu.monitor > /dev/null
    sleep 5

    # QEMU or GDB still running?
    kill_qemu expect_not_running

    tests/check_grep.sh tests/$CAM/$TEST.log -q "GUICMD_LOCK_OFF" || return
    tests/check_grep.sh tests/$CAM/$TEST.log -q "SHUTDOWN" || return
    tests/check_grep.sh tests/$CAM/$TEST.log -q "\[MPU\] Shutdown requested." || return
    tests/check_grep.sh tests/$CAM/$TEST.log -q "Terminate : Success" || return

    tests/check_md5.sh tests/$CAM/ $TEST || cat tests/$CAM/$TEST.md5.log
}

echo
echo "Testing Canon menu..."
for CAM in ${GUI_CAMS[*]}; do
    run_test menu $CAM
done; cleanup


# All GUI cameras should be able to format the virtual card:
function test_format {

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &> tests/$CAM/$TEST.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -vnc :12345 &) \
            &> tests/$CAM/$TEST.log
    fi

    set_gui_timeout
    sleep $GUI_TIMEOUT

    count=0;
    for key in ${FORMAT_SEQUENCE[$CAM]}; do
        if [ $key = wait ]; then sleep 1; continue; fi
        vncdotool -s :12345 key $key; sleep 0.5
        vncdotool -s :12345 capture tests/$CAM/$TEST$((count++)).png
        echo -n .
    done

    kill_qemu expect_running

    tests/check_md5.sh tests/$CAM/ $TEST || cat tests/$CAM/$TEST.md5.log
}

echo
echo "Testing card formatting..."
for CAM in ${GUI_CAMS[*]}; do
    run_test format $CAM
done; cleanup


# All GUI cameras should be able to format the virtual card
# and also restore Magic Lantern:
function test_fmtrestore {

    mkdir -p tests/test-progs/
    wget -q -O tests/test-progs/ml-500D.zip https://builds.magiclantern.fm/jenkins/job/500D.111/431/artifact/platform/500D.111/magiclantern-Nightly.2017Feb12.500D111.zip
    rm -rf tests/test-progs/ml-500D/
    unzip -q -d tests/test-progs/ml-500D/ tests/test-progs/ml-500D.zip
    mcopy -o -s -i $MSD tests/test-progs/ml-500D/* ::

    # check it 3 times (to make sure ML is actually restored)
    # screenshots are slightly different on the first run,
    # runs 2 and 3 must be identical, as the free space
    # no longer depends on the initial card contents.
    count=0;
    for t in 1 2 3; do
        mdel -i $MSD ::/ML/MODULES/LOADING.LCK 2>/dev/null

        if [ -f $CAM/patches.gdb ]; then
            (./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot -vnc :12345 -d debugmsg -s -S & \
                arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &>> tests/$CAM/$TEST.log
        else
            (./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot -vnc :12345 -d debugmsg &) \
                &>> tests/$CAM/$TEST.log
        fi

        sleep 5

        # fixme: how to align these nicely?
        MAIN_SCREEN=2f2febde0863e435fabaed2915661528
        MENU_FORMAT=cae4d8a555d5aa3cc004bd234d3edd74
        FMT_KEEP_ML=077adcdd48ce3c275d94e467f0114045
        FMT_RMOV_ML=a418b9f5d2565f0989910156cbe47c60
        FMT_KEEP_OK=7cdf0d8dd2dde291bca0276cf68694b0
        ML_RESTORED=1a287dd9c3fc75ee82bdb5ba1b30a339
        RESTARTING_=3044730d98d5da3e5e5f27642adf878a

        # fixme: remove duplicate count++, png, break
        T=tests/$CAM/$TEST
        vncexpect f1    $MAIN_SCREEN 30 $T$((count++)).png || break # wait for main info screen with ML loaded
        vncexpect f1    $MAIN_SCREEN 5  $T$((count++)).png || break # also wait for LED activity to settle
        vncexpect f1    $MAIN_SCREEN 5  $T$((count++)).png || break # (the ROM autobackup may take a while)
        vncexpect f1    $MAIN_SCREEN 5  $T$((count++)).png || break # 
        vncexpect m     $MENU_FORMAT 10 $T$((count++)).png || break # MENU
        vncexpect space $FMT_KEEP_ML 20 $T$((count++)).png || break # SET on Format
        vncexpect l     $FMT_RMOV_ML 2  $T$((count++)).png || break # select "remove ML"
        vncexpect l     $FMT_KEEP_ML 2  $T$((count++)).png || break # select "keep ML" on Format
        vncexpect right $FMT_KEEP_OK 2  $T$((count++)).png || break # select OK
        vncexpect space $ML_RESTORED 20 $T$((count++)).png || break # SET, wait for "Magic Lantern restored"
        vncexpect f1    $RESTARTING_ 10 $T$((count++)).png || break # wait for "Restarting camera..."

        # PROP_REBOOT will shutdown the emulator
        # (restarting is not implemented)
        sleep 5

        # QEMU or GDB still running?
        kill_qemu expect_not_running

        if [ $t -eq 3 ]; then
            echo " OK"   # one complete test run, stop here
            return
        else
            echo -n " "
        fi
    done

    kill_qemu
    echo -e "\e[31mFAILED!\e[0m"
}

echo
echo "Testing ML restore after format..."
# fixme: testing hardcoded for 500D
for CAM in 500D; do
    run_test fmtrestore $CAM
done; cleanup


# All cameras should run under GDB and start a few tasks
function test_gdb {

    if [ ! -f $CAM/debugmsg.gdb ]; then
        echo -e "\e[33m$CAM/debugmsg.gdb not present\e[0m"
        return
    fi

    (./run_canon_fw.sh $CAM,firmware="boot=0" -snapshot -display none -s -S & \
     arm-none-eabi-gdb -x $CAM/debugmsg.gdb -ex quit &) &> tests/$CAM/$TEST.log

    # wait for some tasks to start
    touch tests/$CAM/$TEST.log
    ( timeout 2 tail -f -n100000 tests/$CAM/$TEST.log & ) | grep --binary-files=text -qP "task_create\("

    # let it run for 1 second
    sleep 1
    kill_qemu expect_running

    tac tests/$CAM/$TEST.log > tmp
    tests/check_grep.sh tmp -Em1 "task_create\("
    echo -n "         "
    tests/check_grep.sh tmp -Em1 "register_interrupt\([^n]"
}

echo
echo "Testing GDB scripts..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]} ${POWERSHOT_CAMS[*]}; do
    run_test gdb $CAM
done; cleanup


# Models able to display some Canon GUI should capture a still picture as well.
# This requires a full-res silent picture at qemu/<camera>/VRAM/PH-QR/RAW-000.DNG.
# Currently working on 500D, 550D, 50D, 60D, 1200D, and to a lesser extent, on 5D3 and 1100D.
function test_frsp {

    # compile it from ML dir, for each camera
    FRSP_PATH=../magic-lantern/minimal/qemu-frsp
    rm -f $FRSP_PATH/autoexec.bin
    [ $CAM == "1200D" ] && (cd $FRSP_PATH; hg up qemu -C; hg merge 1200D; cd $OLDPWD) &>> tests/$CAM/$TEST-build.log
    make MODEL=$CAM -C $FRSP_PATH clean &>> tests/$CAM/$TEST-build.log
    make MODEL=$CAM -C $FRSP_PATH       &>> tests/$CAM/$TEST-build.log
    [ $CAM == "1200D" ] && (cd $FRSP_PATH; hg up qemu -C; cd $OLDPWD) &>> tests/$CAM/$TEST-build.log
    
    if [ ! -f $FRSP_PATH/autoexec.bin ]; then
        echo -e "\e[31mCompile error\e[0m"
        return
    fi

    # copy autoexec.bin to card images
    mcopy -o -i $MSD $FRSP_PATH/autoexec.bin ::
    mcopy -o -i $MCF $FRSP_PATH/autoexec.bin ::

    # run the photo capture test
    # wait for FA_CaptureTestImage Fin
    (
      ./wait_log.sh tests/$CAM/$TEST-uart.log 20 5 -q --text "FA_CaptureTestImage Fin" 2>/dev/null
      sleep 1
      echo screendump tests/$CAM/$TEST.ppm
      echo quit
    ) | (
      ./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot \
        -display none -monitor stdio \
        -d debugmsg \
        -serial file:tests/$CAM/$TEST-uart.log \
    ) &> tests/$CAM/$TEST.log

    tests/check_grep.sh tests/$CAM/$TEST-uart.log \
        -qm1 "FA_CreateTestImage Fin"  || return

    tests/check_grep.sh tests/$CAM/$TEST-uart.log \
        -qm1 "FA_CaptureTestImage Fin" || return

    tests/check_grep.sh tests/$CAM/$TEST-uart.log \
        -qm1 "FA_DeleteTestImage Fin"  || return

    if [ -f tests/$CAM/$TEST.md5 ]; then
        tests/check_md5.sh tests/$CAM/ $TEST
    else
        echo "OK (no display)"
    fi
}

echo
echo "Testing FA_CaptureTestImage..."
for CAM in ${FRSP_CAMS[*]}; do
    run_test frsp $CAM
done; cleanup


# Most EOS cameras should be able to create the DCIM directory if missing.
function test_dcim {

    # remove the DCIM directory from the card images
    mdeltree -i $MSD ::/DCIM &> /dev/null
    mdeltree -i $MCF ::/DCIM &> /dev/null

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -display none -s -S & \
         arm-none-eabi-gdb -x $CAM/patches.gdb -ex quit &) &> tests/$CAM/$TEST.log
        sleep 5
        kill_qemu expect_running
    else
        (sleep 5; echo quit) \
            | ./run_canon_fw.sh $CAM,firmware="boot=0" -display none -monitor stdio &> tests/$CAM/$TEST.log
    fi
    
    if (mdir -b -i $MSD | grep -q DCIM) || (mdir -b -i $MCF | grep -q DCIM); then
        echo "OK"
    else
        echo -e "\e[31mFAILED!\e[0m"
    fi
}

echo
echo "Testing file I/O (DCIM directory)..."
# Currently works only on models that can boot Canon GUI,
# and also on 1300D.
for CAM in ${GUI_CAMS[*]} 1300D; do
    run_test dcim $CAM
done; cleanup


# All EOS cameras should run the portable display test:
function test_disp {
    (sleep 5; echo screendump tests/$CAM/$TEST.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -snapshot -display none -monitor stdio &> tests/$CAM/$TEST.log
    
    tests/check_md5.sh tests/$CAM/ $TEST
}

echo
echo "Testing display from bootloader..."
for CAM in ${EOS_CAMS[*]}; do
    run_test disp $CAM
done; cleanup


# M3 uses a special CHDK build from Ant123
# using it for the other PowerShot models
# just to test whether they are booting from the card
function test_disp_chdk {
    echo ""

    M3_DISKBOOT_BIN=tests/test-progs/M3/DISKBOOT.BIN
    if [ ! -f $M3_DISKBOOT_BIN ]; then
        mkdir -p `dirname $M3_DISKBOOT_BIN`
        wget -q -O $M3_DISKBOOT_BIN http://a1ex.magiclantern.fm/bleeding-edge/M3/qemu/DISKBOOT.BIN
    fi

    # Our SD image appears to be already bootable for CHDK (?!)
    mcopy -o -i $MSD $M3_DISKBOOT_BIN ::

    # run the emulation
    (
      sleep 10
      echo screendump tests/$CAM/$TEST.ppm
      echo quit
    ) | (
      ./run_canon_fw.sh $CAM -snapshot \
            -display none -monitor stdio \
            -serial file:tests/$CAM/$TEST-uart.log \
    ) &> tests/$CAM/$TEST.log

    printf "    SD boot: "; tests/check_grep.sh tests/$CAM/$TEST-uart.log -om1 "StartDiskboot"
    printf "    RAMboot: "; tests/check_grep.sh tests/$CAM/$TEST-uart.log -om1 "Start Program on RAM"
    printf "    Display: "; tests/check_md5.sh tests/$CAM/ $TEST
}

echo
echo "Testing CHDK display..."
for CAM in ${POWERSHOT_CAMS[*]}; do
    run_test disp_chdk $CAM disp
done; cleanup

# some basic tests for PowerShot models
function test_boot_powershot {
    echo ""

    (./run_canon_fw.sh $CAM -snapshot \
       -display none -d romcpy,int -s -S \
       -serial file:tests/$CAM/$TEST-uart.log & \
     arm-none-eabi-gdb -x $CAM/debugmsg.gdb -ex quit &) &> tests/$CAM/$TEST.log

    # wait for TurnOnDisplay, up to 10 seconds
    touch tests/$CAM/$TEST.log
    ( timeout 10 tail -f -n100000 tests/$CAM/$TEST.log & ) | grep -q "TurnOnDisplay"
    kill_qemu expect_running

    printf "    SD boot: "; tests/check_grep.sh tests/$CAM/$TEST-uart.log -om1 "StartDiskboot"
    printf "    Display: "; tests/check_grep.sh tests/$CAM/$TEST.log -om1 "TurnOnDisplay"
    printf "    ROMcopy: "; tests/check_grep.sh tests/$CAM/$TEST.log -oPm1 "(?<=ROMCPY\]) "

    # print ROMCPY messages before the first interrupt
    # exception: EOS M5 copies interesting stuff after the first interrupt
    ansi2txt < tests/$CAM/$TEST.log \
        | ( [ $CAM == "EOSM5" ] && cat || grep -m1 -B 100000 -E "Taking exception|terminating on signal") \
        | grep -oE "\[ROMCPY\].*" \
        | sed -e "s/\[ROMCPY\]/     /"
}

echo
echo "Testing PowerShot models..."
for CAM in ${POWERSHOT_CAMS[*]}; do
    run_test boot_powershot $CAM boot
done; cleanup


echo
echo "Preparing portable ROM dumper..."

ROM_DUMPER_BIN=tests/test-progs/portable-rom-dumper/autoexec.bin
TMP=tests/tmp

mkdir -p $TMP

if [ ! -f $ROM_DUMPER_BIN ]; then
    mkdir -p `dirname $ROM_DUMPER_BIN`
    wget -q -O $ROM_DUMPER_BIN http://a1ex.magiclantern.fm/debug/portable-rom-dumper/qemu/autoexec.bin
fi

# we don't know whether the camera will use SD or CF, so prepare both
mcopy -o -i $MSD $ROM_DUMPER_BIN ::
mcopy -o -i $MCF $ROM_DUMPER_BIN ::

# save the listing of the root filesystem
mdir -i $MSD > $TMP/sd.lst
mdir -i $MCF > $TMP/cf.lst
if ! diff $TMP/sd.lst $TMP/cf.lst ; then
    echo "Error: SD and CF contents do not match."
    exit
fi

function check_rom_md5 {
    # check ROM dumps from SD/CF card image (the dumper saves MD5 checksums)

    # we don't know yet which image was used (CF or SD)
    if   mdir -i $MSD ::ROM* &> /dev/null; then
        DEV=$MSD
    elif mdir -i $MCF ::ROM* &> /dev/null; then
        DEV=$MCF
    else
        echo -e "\e[31mROMs not saved\e[0m"
        return
    fi

    # only one card should contain the ROMs
    if mdir -i $MCF ::ROM* &> /dev/null && mdir -i $MSD ::ROM* &> /dev/null; then
        echo -e "\e[31mROMs on both CF and SD\e[0m"
        return
    fi

    # copy the ROM files locally to check them
    rm -f $TMP/ROM*
    mcopy -i $DEV ::ROM* $TMP/

    # check the MD5 sums
    cd $TMP/
    if md5sum --strict --status -c *.MD5; then
        # OK: print MD5 output normally
        md5sum -c ROM*.MD5 | tr '\n' '\t'
        echo ""
    else
        # not OK: print the status of each file, in red
        echo -n -e "\e[31m"
        md5sum -c ROM*.MD5 2>/dev/null | tr '\n' '\t'
        echo -e "\e[0m"
    fi
    cd $OLDPWD

    # delete the ROM files from the SD/CF images
    if mdir -i $MSD ::ROM* &> /dev/null; then mdel -i $MSD ::ROM*; fi
    if mdir -i $MCF ::ROM* &> /dev/null; then mdel -i $MCF ::ROM*; fi

    # check whether other files were created/modified (shouldn't be any)
    mdir -i $MSD > $TMP/sd2.lst
    mdir -i $MCF > $TMP/cf2.lst
    diff -q $TMP/sd.lst $TMP/sd2.lst
    diff -q $TMP/cf.lst $TMP/cf2.lst
}

# Most EOS cameras should run the portable ROM dumper.
function test_romdump {

    # The dumper requires the "Open file for write" string present in the firmware.
    if ! grep -q "Open file for write" $CAM/ROM[01].BIN ; then
        echo "skipping"
        return
    fi

    # make sure there are no ROM files on the card
    if mdir -i $MSD ::ROM* &> /dev/null; then
        echo "Error: SD image already contains ROM dumps."
        return
    fi
    if mdir -i $MCF ::ROM* &> /dev/null; then
        echo "Error: CF image already contains ROM dumps."
        return
    fi

    # for some reason, 7D is slower
    [ $CAM == "7D" ] && timeout=40 || timeout=20

    (sleep $timeout; echo screendump tests/$CAM/$TEST.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/$TEST.log
    
    check_rom_md5 $CAM
}

echo "Testing portable ROM dumper..."
for CAM in ${EOS_CAMS[*]}; do
    run_test romdump $CAM
done; cleanup

# custom SD image no longer needed
sd_restore
trap - EXIT

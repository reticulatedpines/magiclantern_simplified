#!/bin/bash

# Emulator tests
# This also shows the emulation state on various cameras

EOS_CAMS=( 5D 5D2 5D3 5D4 6D 7D 7D2M
           40D 50D 60D 70D 80D
           400D 450D 500D 550D 600D 650D 700D 750D 760D
           100D 1000D 1100D 1200D 1300D EOSM EOSM2 )

POWERSHOT_CAMS=( EOSM3 EOSM10 EOSM5 A1100 )

EOS_SECONDARY_CORES=( 5D3eeko 5D4AE 7D2S )

GUI_CAMS=( 5D2 5D3 50D 60D 70D 500D 550D 600D 700D 100D 1100D 1200D )
MENU_CAMS=( 5D2 50D 60D 500D 550D 600D 700D 100D 1100D 1200D )
SD_CAMS=( 5D3 5D4 6D 60D 70D 80D 450D 500D 550D 600D 650D 700D 750D 760D
           100D 1000D 1100D 1200D 1300D EOSM EOSM2 )
CF_CAMS=( 5D 5D2 5D3 5D4 7D 7D2M 40D 50D 400D )

declare -A MENU_SEQUENCE
MENU_SEQUENCE[5D2]="f1 left space i i i m up up up space m m w w p p"
MENU_SEQUENCE[50D]="f1 left space i i i m up space space m w w p p"
MENU_SEQUENCE[60D]="f1 i i i i m left left up space m m p p"
MENU_SEQUENCE[500D]="f1 m i i right right up m p p"
MENU_SEQUENCE[550D]="m i i right right down down down space space p p" # info screen not working
MENU_SEQUENCE[600D]="i i m right right p p" # starts with sensor cleaning animation; no info screen?
MENU_SEQUENCE[700D]="f1 m right right p p" # starts in movie mode, no lens
MENU_SEQUENCE[100D]="f1 left space i i i m right up up space up space p p" # starts with date/time screen
MENU_SEQUENCE[1100D]="f1 left space i i m i i left m p p down right space right right space up right space" # starts with date/time screen; drive mode not working
MENU_SEQUENCE[1200D]="f1 left space i i m i i space m m p p down right space right right space up right space" # starts with date/time screen; drive mode not working

FMT_SEQ="space right space wait f1 space"
# these are customized for my ROM dumps (keys required to select the Format menu)
# TODO: some generic way to navigate to Format menu?
declare -A FORMAT_SEQUENCE
FORMAT_SEQUENCE[5D2]="m right right right right $FMT_SEQ"
FORMAT_SEQUENCE[50D]="m down down $FMT_SEQ"
FORMAT_SEQUENCE[60D]="m left left left left $FMT_SEQ"
FORMAT_SEQUENCE[500D]="m $FMT_SEQ"
FORMAT_SEQUENCE[550D]="m $FMT_SEQ"
FORMAT_SEQUENCE[600D]="m right right right $FMT_SEQ"
FORMAT_SEQUENCE[700D]="m right right right right $FMT_SEQ"  # fixme: free space wrong before format
FORMAT_SEQUENCE[100D]="m $FMT_SEQ"                          # fixme: free space wrong before format
FORMAT_SEQUENCE[1100D]="m right right down $FMT_SEQ"
FORMAT_SEQUENCE[1200D]="m left left $FMT_SEQ"

if false ; then
    # to test only specific models
    EOS_CAMS=(5D)
    POWERSHOT_CAMS=()
    MENU_CAMS=(50D 5D2)
fi

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
./run_canon_fw.sh help > build.log

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
    killall -TERM -w qemu-system-arm
}

echo
echo "Testing call/return trace until the first interrupt"
echo "and some basic consistency checks 1 second after..."

# Interrupts are (currently) non-deterministic, so we'll test
# the call/return trace until the first interrupt is enabled.

# Eeko is also a good target for testing call/return trace, because:
# - simple Thumb code from the bootloader (unlike DIGIC 6 cameras,
#   where the bootloader is plain ARM)
# - the overall structure is different enough to cause issues

for CAM in ${EOS_SECONDARY_CORES[*]} ${EOS_CAMS[*]}; do
    printf "%7s: " $CAM

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/calls-fint*.log

    # log all function calls/returns and interrupts
    ./run_canon_fw.sh $CAM,firmware="boot=0" \
        -display none -d calls,tasks,io,int -serial stdio \
        > tests/$CAM/calls-fint-uart.log \
        2> tests/$CAM/calls-fint-raw.log &
    sleep 0.2
    (timeout 10 tail -f -n100000 tests/$CAM/calls-fint-raw.log & ) | grep -q "Enabled interrupt "
    sleep 1
    kill_qemu

    # trim the log file until the first interrupt
    # (in other words, extract the deterministic part)
    cat tests/$CAM/calls-fint-raw.log \
        | grep --binary-files=text -m1 -B 100000 "Enabled interrupt " \
        > tests/$CAM/calls-fint-trim.log

    # extract call/return lines (before any interrupts)
    # note: task switches before interrupts only happen on VxWorks models
    ansi2txt < tests/$CAM/calls-fint-trim.log \
        | grep -E "call |return |Task switch " \
        > tests/$CAM/calls-fint.log

    if grep -qE "([KR].* (READY|AECU)|Dry|Boot)" tests/$CAM/calls-fint-uart.log; then
      msg=`grep --text -oEm1 "([KR].* (READY|AECU)|Dry|Boot)[a-zA-Z >]*" tests/$CAM/calls-fint-uart.log`
      printf "%-16s" "$msg"
    else
      echo -en "\e[33mBad output\e[0m      "
    fi
    echo -n ' '

    ints=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep -E "interrupt *at " | grep -v "return" | wc -l`
    reti=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep "return from interrupt" | wc -l`
    nints=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep -E " interrupt *at " | grep -v "return" | wc -l`
    nreti=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep " return from interrupt" | wc -l`
    tsksw=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep "Task switch to " | wc -l`
    tasks=`ansi2txt < tests/$CAM/calls-fint-raw.log | grep -oP "(?<=Task switch to )[^:]*" | sort | uniq | head -3 |  tr '\n' ' '`
    if (( ints == 0 )); then
      echo -en "\e[33mno interrupts\e[0m "
    else
      echo -n "$ints ints"
      if (( reti == 0 )); then
        echo -e ", \e[31mno reti\e[0m"
        continue
      fi
      if (( nints != 0 )); then echo -n " ($nints nested)"; fi
      echo -n ", $reti reti"
      if (( nreti != 0 )); then echo -n " ($nreti nested)"; fi
      if (( ints - nints > reti + 1 )); then
          echo -en " \e[33mtoo few reti\e[0m "
      fi
      if (( reti > ints )); then
          echo -e " \e[31mtoo many reti\e[0m"
          continue
      fi
      if (( tsksw > 0 )); then
         echo -n ", $tsksw task switches ( $tasks) "
      else
         echo -en ", \e[33mno task switches\e[0m "
      fi
    fi

    tests/check_md5.sh tests/$CAM/ calls-fint || cat tests/$CAM/calls-fint.md5.log
done

echo
echo "Testing unique calls (sorted IDC) until emulation settles..."

# Even with some non-determinisim from interrupts, the functions called
# should be the same (maybe in different order). Therefore, sorting
# the IDC file should give consistent results.
#
# Unfortunately, this only works "most of the time", but not always;
# for now, let's retry up to 5 times until the test succeeds (fixme).

for CAM in 5D3eeko ${EOS_CAMS[*]}; do
  for k in 1 2 3 4 5; do
    printf "%7s: " $CAM

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/calls-sorted*.log

    # log all function calls/returns and export to IDC
    ./run_canon_fw.sh $CAM,firmware="boot=0" \
        -display none -d idc -serial stdio \
        > tests/$CAM/calls-sorted-uart.log \
        2> tests/$CAM/calls-sorted-raw.log &

    # wait until the IDC file no longer grows
    # (hopefully the emulation settled somehow)
    size_after=
    changed=0
    unchanged=0
    for i in `seq 1 30`; do
        sleep 1
        size_before=$size_after
        size_after=`stat --printf="%s" $CAM.idc`
        if [[ $size_before == $size_after ]]; then
            echo -n "."
            unchanged=$((unchanged+1))
            if (( unchanged > 1 + changed/2 )); then
              echo -n " "
              break;
            fi
        else
            echo -n "+"
            changed=$((changed+1))
        fi
    done

    kill_qemu || continue

    # extract only the call address from IDC
    # and sort the file, because our emulation is not deterministic
    cat $CAM.idc | grep -o "MakeFunction(.*)" \
        | sort > tests/$CAM/calls-sorted.idc

    tests/check_md5.sh tests/$CAM/ calls-sorted && break
  done
done

echo
echo "Testing call/return trace on fromutility..."

# We should get a valid call/return trace on the bootloader,
# which loads FROMUTILITY if autoexec.bin is not present on a bootable card.
# There are no timed interrupts here, so the process should be deterministic.
# Note: our CF emulation is not deterministic, so we'll test only SD models.
# The results are assumed to be correct, but they were not thoroughly checked.
# Feel free to report bugs, corner cases and so on.

# remove autoexec.bin from card images (to get the FROMUTILITY menu)
mdel -i $MSD ::/autoexec.bin
mdel -i $MCF ::/autoexec.bin

for CAM in ${SD_CAMS[*]}; do
    printf "%5s: " $CAM

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/calls-from*.log

    # log all function calls/returns and export to IDC
    ./run_canon_fw.sh $CAM,firmware="boot=1" \
        -display none -d calls,idc -serial stdio \
        > tests/$CAM/calls-from-uart.log \
        2> tests/$CAM/calls-from-raw.log &
    sleep 0.2
    (timeout 20 tail -f -n100000 tests/$CAM/calls-from-uart.log & ) | grep -q "FROMUTIL"
    sleep 0.5
    kill_qemu

    # extract call/return lines
    # remove infinite loop at the end, if any
    cat tests/$CAM/calls-from-raw.log \
        | grep -E "call |return " \
        | python remove_end_loop.py \
        > tests/$CAM/calls-from.log

    # extract only the basic info (call address indented, return address)
    # useful for checking when the log format changes
    # fixme: how to transform "return 0xVALUE to 0xADDR" into "return to 0xADDR" with shell scripting?
    cat tests/$CAM/calls-from.log | grep -oP " *call 0x[0-9A-F]+| to 0x[0-9A-F]+" \
        > tests/$CAM/calls-from-basic.log

    # also copy the IDC file for checking its MD5
    # this might work on CF models too, even if some nondeterminism is present (not tested)
    cp $CAM.idc tests/$CAM/calls-from.idc

    # extract only the call address from IDC
    # useful for checking when some additional info changes (e.g. comments)
    cat tests/$CAM/calls-from.idc | grep -o "MakeFunction(.*)" \
        > tests/$CAM/calls-from-basic.idc

    if grep -q "FROMUTIL" tests/$CAM/calls-from-uart.log; then
      grep -oEm1 "FROMUTIL[^*]*" tests/$CAM/calls-from-uart.log | tr -d '\n'
    elif grep -q "AUTOEXEC.BIN not found" tests/$CAM/calls-from-uart.log; then
        echo -en "\e[33mFROMUTILITY not executed  \e[0m"
    else
        echo -e "\e[31mFAILED!\e[0m"
        continue
    fi
    echo -n ' '
    tests/check_md5.sh tests/$CAM/ calls-from || cat tests/$CAM/calls-from.md5.log
done

# -d callstack triggered quite a few nondeterministic assertions during development
# so, running it on all cameras where Canon menu can be navigated should be a good test
# logging the entire call/return trace is possible, but very slow and not repeatable
echo
echo "Testing Canon menu with callstack enabled..."
for CAM in ${MENU_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/menu*[0-9].png
    rm -f tests/$CAM/menu.log

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 -d callstack -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb &) &> tests/$CAM/menu.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 -d callstack &) \
            &> tests/$CAM/menu.log
    fi

    set_gui_timeout
    sleep $((2*$GUI_TIMEOUT))

    count=0;
    for key in ${MENU_SEQUENCE[$CAM]}; do
        # some GUI operations are very slow under -d callstack (many small functions called)
        # for most of them, 1 second is enough, but the logic would be more complex
        vncdotool -s :12345 key $key; sleep 3
        vncdotool -s :12345 capture tests/$CAM/menu$((count++)).png
        echo -n .
    done

    kill_qemu

    tests/check_md5.sh tests/$CAM/ menu || cat tests/$CAM/menu.md5.log
done

# re-create the card images
rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

# All EOS cameras should emulate the bootloader
# and jump to main firmware.
# Also list blocks copied to RAM during startup, if any.
echo
echo "Testing bootloaders..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    # sorry, couldn't get the monitor working together with log redirection...
    # going to wait for red DY (from READY), with 2-second timeout, then kill qemu
    (./run_canon_fw.sh $CAM,firmware="boot=0" -display none -d romcpy &> tests/$CAM/boot.log) &
    sleep 0.2
    ( timeout 2 tail -f -n100000 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31mD\x1B\x5B0m\x1B\x5B31mY\x1B\x5B0m"
    kill_qemu
    
    tests/check_grep.sh tests/$CAM/boot.log -oE "([KR].* (READY|AECU).*|Intercom.*|Dry>)"
    ansi2txt < tests/$CAM/boot.log | grep -oE "\[ROMCPY\].*" | sed -e "s/\[ROMCPY\]/      /"
done

# All EOS cameras should load autoexec.bin, run HPTimer functions
# and print current task name (this actually tests some ML stubs)
echo
echo "Testing HPTimer and task name..."
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/hptimer.log
    rm -f tests/$CAM/hptimer-build.log

    # compile it from ML dir, for each camera
    HPTIMER_PATH=../magic-lantern/minimal/qemu-hptimer
    rm -f $HPTIMER_PATH/autoexec.bin
    make MODEL=$CAM -C $HPTIMER_PATH clean &>> tests/$CAM/hptimer-build.log
    make MODEL=$CAM -C $HPTIMER_PATH       &>> tests/$CAM/hptimer-build.log
    
    if [ ! -f $HPTIMER_PATH/autoexec.bin ]; then
        echo -e "\e[31mCompile error\e[0m"
        continue
    fi

    # copy autoexec.bin to card images
    mcopy -o -i $MSD $HPTIMER_PATH/autoexec.bin ::
    mcopy -o -i $MCF $HPTIMER_PATH/autoexec.bin ::

    # run the HPTimer test
    (./run_canon_fw.sh $CAM,firmware="boot=1" -display none &> tests/$CAM/hptimer.log) &
    sleep 0.2
    ( timeout 1 tail -f -n100000 tests/$CAM/hptimer.log & ) | grep --binary-files=text -qP "\x1B\x5B34mH\x1B\x5B0m\x1B\x5B34me\x1B\x5B0m"
    sleep 1
    kill_qemu
    
    tests/check_grep.sh tests/$CAM/hptimer.log -m1 "Hello from task run_test"
    printf "       "
    tests/check_grep.sh tests/$CAM/hptimer.log -m1 "Hello from HPTimer" && continue
    printf "       "
    tests/check_grep.sh tests/$CAM/hptimer.log -m1 "Hello from task init" && continue
done

# These cameras should be able to navigate Canon menu:
echo
echo "Testing Canon menu..."
for CAM in ${MENU_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/menu*[0-9].png
    rm -f tests/$CAM/menu.log

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb &) &> tests/$CAM/menu.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 &) \
            &> tests/$CAM/menu.log
    fi

    set_gui_timeout
    sleep $GUI_TIMEOUT

    count=0;
    for key in ${MENU_SEQUENCE[$CAM]}; do
        vncdotool -s :12345 key $key; sleep 0.5
        vncdotool -s :12345 capture tests/$CAM/menu$((count++)).png
        echo -n .
    done

    kill_qemu

    tests/check_md5.sh tests/$CAM/ menu || cat tests/$CAM/menu.md5.log
done

# These cameras should be able to format the virtual card:
echo
echo "Testing card formatting..."
for CAM in ${MENU_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/format*[0-9].png
    rm -f tests/$CAM/format.log

    # re-create the card images before each test
    rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 -s -S & \
            arm-none-eabi-gdb -x $CAM/patches.gdb &) &> tests/$CAM/format.log
    else
        (./run_canon_fw.sh $CAM,firmware="boot=0" -vnc :12345 &) \
            &> tests/$CAM/format.log
    fi

    set_gui_timeout
    sleep $GUI_TIMEOUT

    count=0;
    for key in ${FORMAT_SEQUENCE[$CAM]}; do
        if [ $key = wait ]; then sleep 1; continue; fi
        vncdotool -s :12345 key $key; sleep 0.5
        vncdotool -s :12345 capture tests/$CAM/format$((count++)).png
        echo -n .
    done

    kill_qemu

    tests/check_md5.sh tests/$CAM/ format || cat tests/$CAM/format.md5.log
done

# These cameras should be able to format the virtual card
# and also restore Magic Lantern:
echo
echo "Testing ML restore after format..."
TST=fmtrestore
for CAM in 500D; do
    # re-create the card images
    rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/$TST*.png
    rm -f tests/$CAM/$TST.log

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
            (./run_canon_fw.sh $CAM,firmware="boot=1" -vnc :12345 -s -S & \
                arm-none-eabi-gdb -x $CAM/patches.gdb &) &>> tests/$CAM/$TST.log
        else
            (./run_canon_fw.sh $CAM,firmware="boot=1" -vnc :12345 &) \
                &>> tests/$CAM/$TST.log
        fi

        sleep 5

        # fixme: how to align these nicely?
        MAIN_SCREEN=d2ab306b1db2ffb1229a6e86542e24ac
        MENU_FORMAT=cae4d8a555d5aa3cc004bd234d3edd74
        if [ $t -eq 1 ]; then
            FMT_KEEP_ML=077adcdd48ce3c275d94e467f0114045
            FMT_RMOV_ML=a418b9f5d2565f0989910156cbe47c60
            FMT_KEEP_OK=7cdf0d8dd2dde291bca0276cf68694b0
        else
            FMT_KEEP_ML=303015f7866ef679ec4f6bfed576db54
            FMT_RMOV_ML=511b286bfb698b5ad1543429e26c9ebe
            FMT_KEEP_OK=3cd45fb4f2d79b75c919d07d68c1bc4d
        fi
        ML_RESTORED=1a287dd9c3fc75ee82bdb5ba1b30a339
        RESTARTING_=3044730d98d5da3e5e5f27642adf878a

        # fixme: remove duplicate count++, png, break
        T=tests/$CAM/$TST
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

        kill_qemu

        if [ $t -eq 3 ]; then
            echo " OK"   # one complete test run, stop here
            break 2      # (break the "for k" loop)
        else
            echo -n " "
        fi
    done

    kill_qemu
    echo -e "\e[31mFAILED!\e[0m"
done

# All cameras should run under GDB and start a few tasks
echo
echo "Testing GDB scripts..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]} ${POWERSHOT_CAMS[*]}; do
    printf "%5s: " $CAM

    if [ ! -f $CAM/debugmsg.gdb ]; then
        echo -e "\e[33m$CAM/debugmsg.gdb not present\e[0m"
        continue
    fi

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/gdb.log
    (./run_canon_fw.sh $CAM,firmware="boot=0" -display none -s -S & \
     arm-none-eabi-gdb -x $CAM/debugmsg.gdb &) &> tests/$CAM/gdb.log
    sleep 0.5
    ( timeout 2 tail -f -n100000 tests/$CAM/gdb.log & ) | grep --binary-files=text -qP "task_create\("
    sleep 1
    kill_qemu

    tac tests/$CAM/gdb.log > tmp
    tests/check_grep.sh tmp -Em1 "task_create\("
done

# re-create the card images, just in case
rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

echo
echo "Testing FA_CaptureTestImage..."
# Models able to display some Canon GUI should capture a still picture as well.
# This requires a full-res silent picture at qemu/<camera>/VRAM/PH-QR/RAW-000.DNG.
# Currently working on 60D and 1200D.
for CAM in 5D3 60D 1200D; do
    printf "%5s: " $CAM

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/frsp.ppm
    rm -f tests/$CAM/frsp.log
    rm -f tests/$CAM/frsp-build.log

    # compile it from ML dir, for each camera
    FRSP_PATH=../magic-lantern/minimal/qemu-frsp
    rm -f $FRSP_PATH/autoexec.bin
    [ $CAM == "1200D" ] && (cd $FRSP_PATH; hg up qemu -C; hg merge 1200D; cd $OLDPWD) &>> tests/$CAM/frsp-build.log
    make MODEL=$CAM -C $FRSP_PATH clean &>> tests/$CAM/frsp-build.log
    make MODEL=$CAM -C $FRSP_PATH       &>> tests/$CAM/frsp-build.log
    [ $CAM == "1200D" ] && (cd $FRSP_PATH; hg up qemu -C; cd $OLDPWD) &>> tests/$CAM/frsp-build.log
    
    if [ ! -f $FRSP_PATH/autoexec.bin ]; then
        echo -e "\e[31mCompile error\e[0m"
        continue
    fi

    # copy autoexec.bin to card images
    mcopy -o -i $MSD $FRSP_PATH/autoexec.bin ::
    mcopy -o -i $MCF $FRSP_PATH/autoexec.bin ::

    # run the photo capture test
    (sleep 10; echo screendump tests/$CAM/frsp.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/frsp.log
    
    tests/check_md5.sh tests/$CAM/ frsp
done

echo
echo "Testing file I/O (DCIM directory)..."
# Most EOS cameras should be able to create the DCIM directory if missing.
# Currently works only on models that can boot Canon GUI,
# and also on EOSM and 450D.
#for CAM in ${EOS_CAMS[*]}; do
for CAM in ${GUI_CAMS[*]} EOSM 450D; do
    printf "%5s: " $CAM
    
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/dcim.log

    # remove the DCIM directory from the card images
    mdeltree -i $MSD ::/DCIM &> /dev/null
    mdeltree -i $MCF ::/DCIM &> /dev/null

    if [ -f $CAM/patches.gdb ]; then
        (./run_canon_fw.sh $CAM,firmware="boot=0" -display none -s -S & \
         arm-none-eabi-gdb -x $CAM/patches.gdb &) &> tests/$CAM/dcim.log
        sleep 5
        kill_qemu
    else
        (sleep 5; echo quit) \
            | ./run_canon_fw.sh $CAM,firmware="boot=0" -display none -monitor stdio &> tests/$CAM/dcim.log
    fi
    
    if (mdir -b -i $MSD | grep -q DCIM) || (mdir -b -i $MCF | grep -q DCIM); then
        echo "OK"
    else
        echo -e "\e[31mFAILED!\e[0m"
    fi
done

# re-create the card images, just in case
rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

echo
echo "Testing display from bootloader..."

# All EOS cameras should run the portable display test:
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/disp.ppm
    rm -f tests/$CAM/disp.log
    (sleep 5; echo screendump tests/$CAM/disp.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/disp.log
    
    tests/check_md5.sh tests/$CAM/ disp
done

# EOS M3 is different (PowerShot firmware); let's test it too
echo
echo "Testing PowerShot models..."
for CAM in ${POWERSHOT_CAMS[*]}; do
    echo "$CAM:"
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    (./run_canon_fw.sh $CAM -display none -s -S -d romcpy & \
     arm-none-eabi-gdb -x $CAM/debugmsg.gdb &) &> tests/$CAM/boot.log
    sleep 0.5
    ( timeout 10 tail -f -n100000 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31ma\x1B\x5B0m\x1B\x5B31my\x1B\x5B0m"
    kill_qemu

    printf "  SD boot: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "StartDiskboot"
    printf "  Display: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "TurnOnDisplay"
    printf "  ROMcopy: "; tests/check_grep.sh tests/$CAM/boot.log -oPm1 "(?<=ROMCPY\]) "
    ansi2txt < tests/$CAM/boot.log | grep -oE "\[ROMCPY\].*" | sed -e "s/\[ROMCPY\]/   /"
done

echo
echo "Preparing portable ROM dumper..."

# re-create the card images, just in case
rm sd.img; unxz -k sd.img.xz; cp sd.img cf.img

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

echo "Testing portable ROM dumper..."
# Most EOS cameras should run the portable ROM dumper.
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM

    # The dumper requires the "Open file for write" string present in the firmware.
    if ! grep -q "Open file for write" $CAM/ROM[01].BIN ; then
        echo "skipping"
        continue
    fi
    
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/romdump.ppm
    rm -f tests/$CAM/romdump.log

    # make sure there are no ROM files on the card
    if mdir -i $MSD ::ROM* &> /dev/null; then
        echo "Error: SD image already contains ROM dumps."
        continue
    fi
    if mdir -i $MCF ::ROM* &> /dev/null; then
        echo "Error: CF image already contains ROM dumps."
        continue
    fi

    # for some reason, 7D is slower
    [ $CAM == "7D" ] && timeout=40 || timeout=20

    (sleep $timeout; echo screendump tests/$CAM/romdump.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/romdump.log
    
    check_rom_md5 $CAM
done

# custom SD image no longer needed
sd_restore
trap - EXIT

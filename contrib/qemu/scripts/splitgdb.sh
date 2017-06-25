#!/usr/bin/env bash

CAM=${1//,*/}
if [ ! "$CAM" ]; then
    echo usage: same arguments as with run_canon_fw.sh
    echo example: ./splitgdb.sh 60D,firmware="boot=1" -d callstack
    exit
fi

tmux new-session -d "./run_canon_fw.sh $* -s -S"
tmux split-window -h "cgdb -d arm-none-eabi-gdb -x $CAM/debugmsg.gdb"
tmux attach-session -d

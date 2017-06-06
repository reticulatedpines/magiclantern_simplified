#!/usr/bin/env bash

tmux new-session -d "./run_canon_fw.sh $1 -S -s"
tmux split-window -h "arm-none-eabi-gdb -x gdbopts -x $1/debugmsg.gdb"
tmux attach-session -d 


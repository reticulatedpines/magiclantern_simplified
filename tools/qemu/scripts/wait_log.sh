#!/bin/bash
# wait for a string to appear in a log file,
# with timeout since last update of the log file
# and also since starting this script.
#
# note: if the timeout is from the start of the command,
# the solution is much simpler:
# (timeout 10 tail -f -n +1 foo.log & ) | grep -q "string"
#
# arguments: log file, timeout since start, timeout since last update, grep arguments.
# 
# progress indicator - every second, a char is printed:
#   + means the log file was growing
#   . means the log file did not grow
#   all output goes to stderr.

# wait until the log file no longer grows
size_after=
changed=0
unchanged=0
for i in `seq 1 $2`; do
    sleep 1
    if grep "${@:4:99}" $1 1>&2; then
        # string found
        exit 0
    fi
    size_before=$size_after
    size_after=`stat --printf="%s" $1`
    if [[ $size_before == $size_after ]]; then
        echo -n "." >&2
        unchanged=$((unchanged+1))
        if (( unchanged >= $3 )); then
          echo -n " " >&2
          break;
        fi
    else
        echo -n "+" >&2
        changed=$((changed+1))
        unchanged=0
    fi
done

# string not found
exit 1

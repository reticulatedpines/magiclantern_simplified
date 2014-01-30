#!/bin/bash

if [ "x$1" != "x" ]; then
    echo "Trying to merge in specified stubs file: $1";
    cat $1 | grep "^NSTUB" | sed "s/NSTUB(//;s/, /;/;s/).*//;s/^ *//g;s/$/;;/;s/ //g" > stubs.csv
fi

rm *.csv_merge_dat;

# clean up line endings and read files
declare -A filedata
files=0
for src in *.csv; do
    tmp="${src}_merge_dat";
    cat "$src" | grep -v ";nullsub_" | grep -v ";sub_" > "$tmp";
    dos2unix $tmp >/dev/null 2>&1;

    line=0;
    echo "Reading $src..." 1>&2;
    echo "    addresses" 1>&2;
    for line_string in `cat $tmp | sed "s/ /#/g;" | tr '[:lower:]' '[:upper:]' | sed "s/;.*//"`; do
        filedata[${files},${line},0]="${line_string/X/x}";
        line=$(($line+1));
    done
    file_length[${files}]=${line};

    line=0;
    echo "    names" 1>&2;
    for line_string in `cat $tmp | sed "s/^[^;]*;//;s/;.*//"`; do
        filedata[${files},${line},1]="${line_string// /}";
        line=$(($line+1));
    done

    line=0;
    echo "    prototypes" 1>&2;
    for line_string in `cat $tmp | sed "s/ /#/g;" | sed "s/.*;//;" | sed "s/^#*//;"`; do
        filedata[${files},${line},2]="${line_string//#/ }";
        line=$(($line+1));
    done

    files=$(($files+1));
done

# merge all addresses
echo "Reading addresses..." 1>&2;
cat *.csv_merge_dat | gawk -F\; '{print $1; }' | tr '[:lower:]' '[:upper:]' | sort | uniq > funcs.tmp

# output header
echo "Merging..." 1>&2;
echo -ne "Function;Delta;"
for src in *.csv_merge_dat; do
    echo -ne "${src}_use;${src}_name;${src}_proto;";
done
echo -ne "\n";

# now for all functions
for line_string in `cat funcs.tmp`; do
    differs="";
    last=";";
    whole="";

    addr=${line_string/X/x};
    echo -ne "${addr};";

    for ((file=0; file < $files; file++)); do
        str=";";

        # and print them next to each other
        for ((pos=0; pos < ${file_length[$file]}; pos++)); do
            if [ "x$addr" == "x${filedata[${file},${pos},0]}" ]; then
                str="${filedata[${file},${pos},1]};${filedata[${file},${pos},2]}";
            fi
        done

        # check if its there and differs
        if [ "x$str" != "x;" ]; then
            if [ "x$str" != "x$last" -a "x$last" != "x;" ]; then
                differs="X";
            fi
            last="$str";
        fi
        whole="${whole};$str;";
    done

    echo -ne "$differs;$whole\n";
done

rm *.csv_merge_dat;


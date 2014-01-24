#!/bin/bash

##############################################################
# configuration

dump_all=no;
mode=stubs;
#mode=idc;

##############################################################



while getopts ":csia" optname
do
    case "$optname" in
      "c")
        echo "Output: CSV" 1>&2;
        mode=csv;
        ;;
      "s")
        echo "Output: stubs.S" 1>&2;
        mode=stubs;
        ;;
      "i")
        echo "Output: idc script" 1>&2;
        mode=idc;
        ;;
      "a")
        echo "Dumping all merged" 1>&2;
        dump_all=yes;
        ;;
      *)
      # Should not occur
        echo "Unknown error while processing options" 1>&2;
        ;;
    esac
done

shift $(($OPTIND - 1));

if [ "x$1" == "x" ]; then
    echo "You have to pass the input CSV as parameter";
    exit 1;
fi

src=$1;
files=0;
shopt -s extglob
declare -A FILEDATA;


dos2unix $src >/dev/null 2>&1;

line=0;
files=0;

echo "Reading $src" 1>&2;
for line_string in `cat $src | sed "s/ /:/g;s/;:/;/g;s/;;/;:;/g;s/::/:/g;" | sed "s/;;/;:;/g;"`; do
    fields=(${line_string//;/ });

    #echo "fields: ${#fields[@]} from $line_string"
    if [ $files -eq 0 ]; then
        let files=(${#fields[@]}-2)/3;
        echo "Merge of $files files:" 1>&2;
        
        for ((file=0; file < $files; file++)); do
            let pos=$file*3+2;
            filename=${fields[$pos]//:/ };
            filename=${filename##+([[:space:]])}; 
            filename=${filename%%+([[:space:]])};
            filename=(${filename//_/ });
            echo "    File #$file: ${filename[0]}" 1>&2;
        done
    else
        addr=${fields[0]//:/ };
        deltas=${fields[1]//:/};

        if [ "x$deltas" != "x" -o "x$dump_all" == "xyes" ]; then
            found=0;
            for ((file=0; file < $files; file++)); do
                let pos_use=2+$file*3+0;
                let pos_name=2+$file*3+1;
                let pos_proto=2+$file*3+2;
                
                use=${fields[$pos_use]//:/ };
                use=${use##+([[:space:]])}; 
                use=${use%%+([[:space:]])};
                name=${fields[$pos_name]//:/ };
                name=${name##+([[:space:]])}; 
                name=${name%%+([[:space:]])};
                proto=${fields[$pos_proto]//:/ };
                proto=${proto##+([[:space:]])}; 
                proto=${proto%%+([[:space:]])};
                proto=${proto/\(/ ${name}(};
                
                #echo "[$addr $name] [$use $name $proto]" 1>&2;
                if [[ ("x$use" != "x") || ("x$dump_all" != "xno" && "x$name" != "x" && $found = 0) ]]; then
                    if [ "x$mode" == "xstubs" ]; then
                        if [ "x$proto" != "x" ]; then
                            echo "/* $proto */";
                        fi
                        echo "NSTUB($addr, $name)";
                    elif  [ "x$mode" == "xidc" ]; then
                        echo "    MakeNameEx($addr, \"$name\", SN_NOCHECK);";
                        if [ "x$proto" != "x" ]; then
                            echo "    SetType($addr, \"$proto\");";
                        fi
                    elif  [ "x$mode" == "xcsv" ]; then
                        echo "$addr;$name;$proto;";
                    fi
                    found=1;
                fi
            done
            
            if [ $found -eq 0 ]; then
                echo "   WARNING: you did not select the function you want: [$addr $name]" 1>&2;
            fi
        fi
        line=$(($line+1));
    fi
done
file_length[${files}]=${line};

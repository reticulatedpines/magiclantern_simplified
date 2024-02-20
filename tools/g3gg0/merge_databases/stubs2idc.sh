#!/bin/bash

gcc merge_csv.c -o merge_csv --std=c99

echo "### reading $1 into .csv ###"
./merge_csv $1 > tmp.csv
echo "### converting .csv into .idc ###"
./compile_csv.sh -i tmp.csv > ${1}.idc
rm tmp.csv
echo "### DONE ###"


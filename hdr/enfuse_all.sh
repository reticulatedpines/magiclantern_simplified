#!/bin/bash

cd frames/
for f in $(ls A*.jpg); do
    echo $f "(+)" B${f:1} "=>" C${f:1};  
    enfuse -o C${f:1} $f B${f:1};  
done

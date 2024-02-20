#!/usr/bin/env bash
cd $1

if [ ! -e $2.md5 ]; then
  echo -e "\e[33mplease check\e[0m"
  exit
fi

(md5sum -c $2.md5 &> $2.md5.log) && echo "OK" || (echo -e "\e[31mFAILED!\e[0m"; exit 1)

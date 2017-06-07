#!/usr/bin/env bash
(ansi2txt < $1 | grep $2 "$3") || echo -e "\e[31mFAILED!\e[0m"

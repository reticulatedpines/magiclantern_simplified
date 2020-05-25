#!/usr/bin/env bash
(ansi2txt < $1 | grep "${@:2}") || (echo -e "\e[31mFAILED!\e[0m"; exit 1)

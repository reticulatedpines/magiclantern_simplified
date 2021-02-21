@echo off

call ..\cfg.bat 


gmake

copy check_sum.exe f:\_tools\check_sum.exe


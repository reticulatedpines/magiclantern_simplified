@echo off

call ..\cfg.bat 


gmake

copy eos_fsum.exe f:\_tools\eos_fsum.exe


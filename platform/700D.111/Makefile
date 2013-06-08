#Makefile for 700D

MODEL=700D
include ../Makefile.platform

x: 
	make clean
	make autoexec-fir.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR autoexec-fir.bin ml-700D-111.fir

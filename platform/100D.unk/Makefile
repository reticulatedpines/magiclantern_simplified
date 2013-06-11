#Makefile for 100D

MODEL=100D
include ../Makefile.platform

x: 
	make clean
	make autoexec-fir.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR autoexec-fir.bin ml-5D3-113.fir

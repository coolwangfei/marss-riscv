#!/bin/sh
export LD_LIBRARY_PATH=../src:../src/DRAMSim2
../src/marss-riscv -mem-model base ./marss-riscv-images/config.cfg
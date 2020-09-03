#!/bin/bash
riscv64-unknown-elf-as test.s -o test.elf
riscv64-unknown-elf-objcopy -O binary test.elf test.bin
riscv64-unknown-elf-objdump -D test.elf > test.asm

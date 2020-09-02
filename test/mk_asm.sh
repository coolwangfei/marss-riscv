#!/bin/bash
riscv64-unknown-linux-gnu-as test.asm -o test.elf
riscv64-unknown-elf-objcopy -O binary test.elf test.bin
riscv64-unknown-elf-objdump -D test.elf > test.a

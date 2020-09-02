#!/bin/bash
riscv64-unknown-elf-gcc -nostdinc -nostdlib --entry main bare.c -o bare
riscv64-unknown-elf-objcopy -O binary bare bare.bin
riscv64-unknown-elf-objdump -D bare > bare.s

riscv64-unknown-elf-as -o asm asm.S

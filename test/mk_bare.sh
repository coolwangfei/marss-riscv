#!/bin/bash
riscv64-unknown-elf-gcc -nostdinc -nostdlib -march=rv64g -mabi=lp64d --entry main bare.c -o bare
riscv64-unknown-elf-objcopy -O binary bare bare.bin
riscv64-unknown-elf-objdump -D bare > bare.asm


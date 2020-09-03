#!/bin/bash
riscv64-unknown-elf-gcc -nostdinc -nostdlib --entry main bare.c -o bare
riscv64-unknown-elf-objcopy -O binary bare bare.bin
<<<<<<< HEAD
riscv64-unknown-elf-objdump -D bare > bare.asm
=======
riscv64-unknown-elf-objdump -D bare > bare.s

riscv64-unknown-elf-as -o asm asm.S
>>>>>>> 918b54e7159a0b82a66558a3cdf09c268857ab65

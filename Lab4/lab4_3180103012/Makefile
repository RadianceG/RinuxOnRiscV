# Makefile_lab4
# ln -s ~/lab4/.gdbinit ~/.gdbinit
# sed -i '$aalias md=make\\ \\&\\&\\ make\\ debug' ~/.bashrc
# sed -i '$aalias gdb=/opt/riscv/bin/riscv64-unknown-linux-gnu-gdb' ~/.bashrc
# sed -i '$aalias gv=/opt/riscv/bin/riscv64-unknown-linux-gnu-gdb\\ vmlinux' ~/.bashrc
export TOP=$(shell pwd)
export RISCV=/opt/riscv
export PATH:=${PATH}:${RISCV}/bin

export
CROSS_=riscv64-unknown-elf-
AR=${CROSS_}ar
CC=${CROSS_}gcc
LD=${CROSS_}ld
OBJCOPY=${CROSS_}objcopy
OBJDUMP=${CROSS_}objdump
READELF=${CROSS_}readelf

ISA ?= rv64imafd
ABI ?= lp64

INCLUDE = -I ${TOP}/include/ -I ${TOP}/arch/riscv/include/
CF = -O0 -march=$(ISA) -mabi=$(ABI) -mcmodel=medany -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nostdinc -static -lgcc -Wl,--nmagic -Wl,--gc-sections -g
CFLAG = ${CF} ${INCLUDE} -D SJF
# CFLAG = ${CF} ${INCLUDE} -D PRIORITY

.PHONY: all
all:
	@make -C ${TOP}/lib/
	@make -C ${TOP}/init/
	@make -C ${TOP}/arch/riscv/kernel/
	@make -C ${TOP}/arch/riscv/
	@echo "\e[35m==== Build Successfully ====\e[0m"
	${OBJDUMP} -S ${TOP}/vmlinux > ${TOP}/misc/dasm.S
	${READELF} -S ${TOP}/vmlinux > ${TOP}/misc/section.txt
	@echo "\e[35m==== DeAsm Successfully ====\e[0m"

.PHONY: run
run:
	qemu-system-riscv64 -nographic -machine virt -kernel vmlinux
	@echo "\e[32m==== Run Successfully ====\e[0m"

.PHONY: debug
debug:
	@echo "\e[32m==== Start Debugging ====\e[0m"
	qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -S -s
	@echo "\e[32m==== End Debugging ====\e[0m"

.PHONY: clean
clean:
	@rm -f ${TOP}/vmlinux ${TOP}/System.map ${TOP}/a.out
	@make -C ${TOP}/init/ clean
	@make -C ${TOP}/lib/ clean
	@make -C ${TOP}/arch/riscv/kernel/ clean
	@make -C ${TOP}/arch/riscv/ clean
	@echo "\e[36m==== Clean Successfully ====\e[0m"

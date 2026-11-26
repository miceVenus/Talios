# CONFIGURE TOOL CHAIN
CC 		:= 	gcc-9
LD 		:= 	ld
OBJCOPY := 	objcopy
AS 		:=	as
NAS 	:= 	nasm
# QEMU	:= 	qemu-system-x86_64 no more QEMU There is some bug on Int 13h
BOCHS 	?= 	bochs
BOCHSFILE 	:= ./bochsrc.floppy
CPUS        ?= 1
CHECK_CPUS  ?= 1 2 4
# Bochs supports up to eight cores per socket.
CPU_TOPOLOGY = $(if $(filter 1 2 3 4 5 6 7 8,$(CPUS)),1:$(CPUS):1,$(CPUS):1:1)

PIC		:= 	APIC
FLOPPY 	:= 	FLOPPY_DMA


CPFLAGS :=  -I elf64-x86-64 -S -R ".eh_frame" -R ".comment"

# <float.h>, <iso646.h>, <limits.h>, <stdalign.h>, <stdarg.h>, 
# <stdbool.h>, <stddef.h>, <stdint.h>, and <stdnoreturn.h> could be useful in freestanding. 
# You should be familiar with these headers as they contain useful declarations you shouldn't do yourself. 
# GCC also comes with additional freestanding headers for CPUID, SSE and such.
CFLAGS 		:= 	-mno-red-zone -mcmodel=large -fno-builtin -m64 -ffreestanding -g -Wall -Wextra -MMD -MP -O0 -fvar-tracking -D$(PIC) -D$(FLOPPY) $(EXTRA_CFLAGS)

LDFLAGS 	:=   -b elf64-x86-64 -z muldefs
ASFLAGS 	:=  
NASFLAGS	:= 


# SOME DIRECTORIES AND SOME FILES
BIOSDIR		:= 	bootloader
KENRELDIR	:= 	kernel
TESTDIR 	:= 	kernel/test
OBJDIR 		:= 	build
BINDIR 		:= 	bin
USERDIR		:= 	user

# THIS IS AN ELF
TARGET		:= 	$(BINDIR)/system.bin
USER_TARGET	:= 	$(BINDIR)/user.bin

KERNELBIN	:= 	$(BINDIR)/kernel.bin
INITBIN		:= 	$(BINDIR)/init.bin

KERNEL_SRCS  	:= 	$(wildcard $(KENRELDIR)/*.c)
USER_SRCS  		:= 	$(wildcard $(USERDIR)/*.c)
AS_SRCS 	:=	$(wildcard $(KENRELDIR)/*.S)
TEST_SRCS 	:=	$(wildcard $(TESTDIR)/*.c)
NAS_SRCS	:= 	$(wildcard $(BIOSDIR)/*.asm)

OBJS    	:= 	$(patsubst $(KENRELDIR)/%.c,$(OBJDIR)/%.o,$(KERNEL_SRCS)) \
           		$(patsubst $(KENRELDIR)/%.S,$(OBJDIR)/%.o,$(AS_SRCS))\
				$(patsubst $(TESTDIR)/%.c,$(OBJDIR)/%.o,$(TEST_SRCS)) \

USER_OBJS	:= 	$(patsubst $(USERDIR)/%.c,$(OBJDIR)/%_user.o,$(USER_SRCS))


TEST		:= 	$(patsubst $(TESTDIR)/%.c,$(OBJDIR)/%.o,$(TEST_SRCS))

BOOTLOADER	:= 	$(patsubst $(BIOSDIR)/%.asm,$(BINDIR)/%.bin,$(NAS_SRCS))

DEPS    	:= 	$(OBJS:.o=.d)

USER_DEPS	:= 	$(USER_OBJS:.o=.d)

#BOOTLOADER ASSMBLE RULE
$(BINDIR)/%.bin: $(BIOSDIR)/%.asm | $(BINDIR)
	$(NAS) $(NASFLAGS) $< -o $@

# KERNEL COMPILE RULE
$(OBJDIR)/%.o: $(KENRELDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@


# USER COMPILE RULE
$(OBJDIR)/%_user.o: $(USERDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# TEST COMPILE RULE
$(OBJDIR)/%.o: $(TESTDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $< -o $@

# HEAD ASSMBLE RULE
$(OBJDIR)/%.o: $(KENRELDIR)/%.S | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# LINKER
$(TARGET): $(OBJS) $(KENRELDIR)/kernel.lds | $(BINDIR)
	$(LD) $(LDFLAGS) -T $(KENRELDIR)/kernel.lds -o $@ $(OBJS)

# FOR USER
$(USER_TARGET): $(USER_OBJS) $(USERDIR)/user.lds | $(BINDIR)
	$(LD) $(LDFLAGS) -T $(USERDIR)/user.lds -o $@ $(USER_OBJS)

$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(KERNELBIN): $(TARGET)
	$(OBJCOPY) $(CPFLAGS) -O binary $< $@

$(INITBIN): $(USER_TARGET) | $(BINDIR)
	$(OBJCOPY) $(CPFLAGS) -O binary $< $@

.PHONY: all
all: $(KERNELBIN) $(BOOTLOADER) $(INITBIN)

.PHONY: check-vm
check-vm:
	python3 checks/run_vm.py --bochs "$(BOCHS)" --cpus $(CPUS)

.PHONY: check-sched
check-sched:
	python3 checks/run_vm.py --sched --bochs "$(BOCHS)" --cpus $(CHECK_CPUS)

.PHONY: vmcheck
vmcheck: $(BINDIR)/vmcheck.bin

$(OBJDIR)/vmcheck.o: checks/vm.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/vmcheck.elf: $(OBJDIR)/vmcheck.o $(OBJDIR)/lib_user.o $(OBJDIR)/malloc_user.o $(USERDIR)/user.lds | $(BINDIR)
	$(LD) $(LDFLAGS) -T $(USERDIR)/user.lds -o $@ $(filter %.o,$^)

$(BINDIR)/vmcheck.bin: $(BINDIR)/vmcheck.elf
	$(OBJCOPY) $(CPFLAGS) -O binary $< $@

.PHONY: schedcheck
schedcheck: $(BINDIR)/schedcheck.bin

$(OBJDIR)/schedcheck.o: checks/sched.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/schedcheck.elf: $(OBJDIR)/schedcheck.o $(OBJDIR)/lib_user.o $(OBJDIR)/printf_user.o $(OBJDIR)/string_user.o $(USERDIR)/user.lds | $(BINDIR)
	$(LD) $(LDFLAGS) -T $(USERDIR)/user.lds -o $@ $(filter %.o,$^)

$(BINDIR)/schedcheck.bin: $(BINDIR)/schedcheck.elf
	$(OBJCOPY) $(CPFLAGS) -O binary $< $@

.PHONY: run
run: all $(BOCHSFILE)
	./assmble.sh
	sed '/^[[:space:]]*gdbstub:/d' $(BOCHSFILE) > $(OBJDIR)/bochsrc.run
	"$(BOCHS)" -q -f $(OBJDIR)/bochsrc.run 'cpu: count=$(CPU_TOPOLOGY)'

.PHONY: debug
debug: all $(BOCHSFILE)
	./assmble.sh
	sed 's/^# gdbstub:/gdbstub:/' $(BOCHSFILE) > $(OBJDIR)/bochsrc.debug
	"$(BOCHS)" -q -f $(OBJDIR)/bochsrc.debug 'cpu: count=$(CPU_TOPOLOGY)'

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)/*.bin

# head: 	$K/head.S
# 	gcc -E $K/head.S > head.s
# 	as 	-o head.o head.s
# 	rm 	head.s

# main: 	$K/main.c
# 	gcc -mcmodel=large -fno-builtin -m64 -ffreestanding -c -o main.o $K/main.c -g3 -O0 -Wall -Wextra -Werror -I.

# system: head main
# 	ld -b elf64-x86-64 -o system head.o main.o -T kernel.lds
	
# all: system
# 	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

# debug: system
# 	objcopy -I elf64-x86-64 -O binary system kernel.bin

# clean: 
# 	rm -f *.o system kernel.bin

-include $(DEPS) $(USER_DEPS) $(OBJDIR)/vmcheck.d $(OBJDIR)/schedcheck.d

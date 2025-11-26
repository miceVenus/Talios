# CONFIGURE TOOL CHAIN
CC 		:= 	gcc
LD 		:= 	ld
OBJCOPY := 	objcopy
AS 		:=	as
NAS 	:= 	nasm
# QEMU	:= 	qemu-system-x86_64 no more QEMU There is some bug on Int 13h
BOCHS 	:= 	bochs
BOCHSFILE 	:= ./bochsrc.floppy

PIC		:= 	PIC


CPFLAGS :=  -I elf64-x86-64 -S -R ".eh_frame" -R ".comment"

# <float.h>, <iso646.h>, <limits.h>, <stdalign.h>, <stdarg.h>, 
# <stdbool.h>, <stddef.h>, <stdint.h>, and <stdnoreturn.h> could be useful in freestanding. 
# You should be familiar with these headers as they contain useful declarations you shouldn't do yourself. 
# GCC also comes with additional freestanding headers for CPUID, SSE and such.
CFLAGS 		:= 	-mcmodel=large -fno-builtin -m64 -ffreestanding -g -Wall -Wextra -MMD -MP -O0 -fvar-tracking -D$(PIC)

LDFLAGS 	:=   -b elf64-x86-64 -z muldefs
ASFLAGS 	:=  
NASFLAGS	:= 


# SOME DIRECTORIES AND SOME FILES
BIOSDIR		:= 	bootloader
KENRELDIR	:= 	kernel
TESTDIR 	:= 	kernel/test
OBJDIR 		:= 	build
BINDIR 		:= 	bin

# THIS IS AN ELF
TARGET		:= 	$(BINDIR)/system.bin

KERNELBIN	:= 	$(BINDIR)/kernel.bin

KERNEL_SRCS  	:= 	$(wildcard $(KENRELDIR)/*.c)
AS_SRCS 	:=	$(wildcard $(KENRELDIR)/*.S)
TEST_SRCS 	:=	$(wildcard $(TESTDIR)/*.c)
NAS_SRCS	:= 	$(wildcard $(BIOSDIR)/*.asm)

OBJS    	:= 	$(patsubst $(KENRELDIR)/%.c,$(OBJDIR)/%.o,$(KERNEL_SRCS)) \
           		$(patsubst $(KENRELDIR)/%.S,$(OBJDIR)/%.o,$(AS_SRCS))\
				$(patsubst $(TESTDIR)/%.c,$(OBJDIR)/%.o,$(TEST_SRCS))
				

TEST		:= 	$(patsubst $(TESTDIR)/%.c,$(OBJDIR)/%.o,$(TEST_SRCS))

BOOTLOADER	:= 	$(patsubst $(BIOSDIR)/%.asm,$(BINDIR)/%.bin,$(NAS_SRCS))

DEPS    	:= 	$(OBJS:.o=.d)

#BOOTLOADER ASSMBLE RULE
$(BINDIR)/%.bin: $(BIOSDIR)/%.asm | $(BINDIR)
	$(NAS) $(NASFLAGS) $< -o $@

# KERNEL COMPILE RULE
$(OBJDIR)/%.o: $(KENRELDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# TEST COMPILE RULE
$(OBJDIR)/%.o: $(TESTDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# HEAD ASSMBLE RULE
$(OBJDIR)/%.o: $(KENRELDIR)/%.S | $(OBJDIR)
	gcc -E $< -o temp.s
	$(AS) $(ASFLAGS) temp.s -o $@
	rm temp.s

# LINKER
$(TARGET): $(OBJS) $(KENRELDIR)/kernel.lds | $(BINDIR)
	$(LD) $(LDFLAGS) -T $(KENRELDIR)/kernel.lds -o $@ $(OBJS)
	
$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(KERNELBIN): $(TARGET)
	$(OBJCOPY) $(CPFLAGS) -O binary $< $@

.PHONY: all
all: $(KERNELBIN) $(BOOTLOADER)

.PHONY: run
run: all $(BOCHSFILE)
	./assmble.sh
	sed -i 's/.*gdbstub/# gdbstub/g' $(BOCHSFILE)
	$(BOCHS) -q -f $(BOCHSFILE)

.PHONY: debug
debug: all $(BOCHSFILE)
	./assmble.sh
	sed -i 's/# gdbstub/gdbstub/g' $(BOCHSFILE)
	$(BOCHS) -q -f $(BOCHSFILE)

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

-include $(DEPS)
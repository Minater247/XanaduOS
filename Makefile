ARCH ?= x86

CC = i686-elf-gcc.exe
LD = i686-elf-ld.exe
AS = i686-elf-as.exe
OBJCOPY = i686-elf-objcopy.exe

SFILES := $(wildcard arch/$(ARCH)/*.s)
CFILES := $(wildcard arch/$(ARCH)/*.c) $(wildcard kernel/*.c) $(wildcard kernel/*/*.c) $(wildcard arch/$(ARCH)/*/*.c)

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iarch/$(ARCH) -Ikernel/include
LDFLAGS = -T arch/$(ARCH)/linker.ld -O2 -nostdlib -ffreestanding -lgcc

OBJ = $(SFILES:.s=.o) $(CFILES:.c=.o)
OBJ_DEBUG = $(SFILES:.s=.dbs) $(CFILES:.c=.dbo)

all: $(OBJ) link

link:
	$(CC) $(LDFLAGS) -o kernel.bin $(OBJ)

all_debug: $(OBJ_DEBUG) link_debug

link_debug:
	$(CC) $(LDFLAGS) -o kernel.bin $(OBJ_DEBUG)

# Depending on the architecture, we need to use different assembly syntax
%.o: %.s
	$(AS) -o $@ $<

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

%.dbo: %.c
	$(CC) -c $< $(CFLAGS) -g -o $@

%.dbs: %.s
	$(AS) -g -o $@ $<

iso: all
# Ensure kernel.bin is multiboot compliant, if not, error
	@if grub-file --is-x86-multiboot kernel.bin; then echo multiboot confirmed; else echo the file is not multiboot; exit 1; fi
# Ensure kernel.bin is <= 8MB, if not, error
	@if [ $$(stat -c%s kernel.bin) -gt 8388608 ]; then echo kernel.bin is too large! Add more pages; exit 1; fi
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	cp arch/$(ARCH)/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir

debug_iso: all_debug
# Ensure kernel.bin is multiboot compliant, if not, error
	@if grub-file --is-x86-multiboot kernel.bin; then echo multiboot confirmed; else echo the file is not multiboot; exit 1; fi
# Ensure kernel.bin is <= 8MB, if not, error
	@if [ $$(stat -c%s kernel.bin) -gt 8388608 ]; then echo kernel.bin is too large! Add more pages; exit 1; fi
	mkdir -p dbg_isodir/boot/grub
	cp kernel.bin dbg_isodir/boot/kernel.bin
	cp arch/$(ARCH)/grub.cfg dbg_isodir/boot/grub/grub.cfg
	grub-mkrescue -o os_dbg.iso dbg_isodir

run: iso
	qemu-system-i386 -cdrom os.iso

# Debugging should output logs to ./q_debug.log
debug: debug_iso
#	qemu-system-i386 -cdrom os_dbg.iso -s -S -monitor stdio -d int -D ./q_debug.log
# use bochs
	"/mnt/c/Program Files/Bochs-2.7/bochsdbg.exe" -f bochsrc.txt -q

debug_term:
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.bin" -ex "target remote localhost:1234"

clean:
	rm -f $(OBJ) $(OBJ_DEBUG) kernel.bin os.iso os_dbg.iso *.o

crun: clean run
cdbg: clean debug
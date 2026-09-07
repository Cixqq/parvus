# Nuke built-in rules
.SUFFIXES:

# Configuration
OUTPUT       := myos
ISO_NAME     := image.iso
ISO_ROOT     := iso_root
LIMINE_DIR   := limine-binary

# Toolchain selection
TOOLCHAIN        ?= llvm
TOOLCHAIN_PREFIX ?=
ifeq ($(TOOLCHAIN),llvm)
    TOOLCHAIN_PREFIX ?= llvm-
    CC               := clang
    LD               := ld.lld
else
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
    CC               := $(if $(TOOLCHAIN_PREFIX),$(TOOLCHAIN_PREFIX)gcc,cc)
    LD               := $(TOOLCHAIN_PREFIX)ld
endif

# User-controllable flags
CFLAGS    ?= -g -O2 -pipe -masm=intel
CPPFLAGS  ?=
NASMFLAGS ?= -g
LDFLAGS   ?=

# Detect if CC is Clang
CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $?)
ifeq ($(CC_IS_CLANG),1)
    CFLAGS += -target x86_64-unknown-none-elf
endif

# Internal C flags (non-negotiable for kernel development)
CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel

# Internal preprocessor flags
CPPFLAGS := -I src/ $(CPPFLAGS) -MMD -MP

# Internal NASM flags
NASMFLAGS := -f elf64 $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) -Wall

# Internal Linker flags
LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections \
    -T linker.lds

# Source file discovery
SRCFILES     := $(shell find -L src/ -type f -not -path "./limine-binary/*" 2>/dev/null | LC_ALL=C sort)
CFILES       := $(filter %.c,$(SRCFILES))
ASFILES      := $(filter %.S,$(SRCFILES))
NASMFILES    := $(filter %.asm,$(SRCFILES))
OBJ          := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
HEADER_DEPS  := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target
.PHONY: all
all: bin/$(OUTPUT)

# Include automatic header dependencies
-include $(HEADER_DEPS)

# --- Build Rules ---

bin/$(OUTPUT): Makefile linker.lds $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

obj/%.c.o: %.c Makefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.S.o: %.S Makefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.asm.o: %.asm Makefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@

# --- Limine & ISO Generation ---

$(LIMINE_DIR):
	git clone https://github.com/Limine-Bootloader/Limine.git --branch=binary --depth=1 $@

.PHONY: limine-tool
limine-tool: $(LIMINE_DIR)
	$(MAKE) -C $(LIMINE_DIR)

.PHONY: iso
iso: $(ISO_NAME)

$(ISO_NAME): bin/$(OUTPUT) limine-tool limine.conf
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT

	cp -v bin/$(OUTPUT) $(ISO_ROOT)/boot/
	cp -v limine.conf $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/limine-bios.sys $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/
	cp -v $(LIMINE_DIR)/BOOTIA32.EFI $(ISO_ROOT)/EFI/BOOT/

	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $@

	./$(LIMINE_DIR)/limine bios-install $@

.PHONY: run
run: $(ISO_NAME)
	qemu-system-x86_64 -cdrom $(ISO_NAME) 2>/dev/null

.PHONY: clean
clean:
	rm -rf bin obj $(ISO_ROOT) $(ISO_NAME)

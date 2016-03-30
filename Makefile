CC ?= gcc
LD ?= gcc

CFLAGS := -g -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -ffreestanding \
	-mcmodel=kernel -Wall -Wextra -Werror -pedantic -std=c99 \
	-Wframe-larger-than=4096 -Wstack-usage=4096 -Wno-unknown-warning-option
LFLAGS := -nostdlib -z max-page-size=0x1000

INTERDIR=.tmp

ASM := bootstrap.S videomem.S int_handler.S threading_asm.S
AOBJ:= $(ASM:%.S=$(INTERDIR)/%.o)
ADEP:= $(ASM:%.S=$(INTERDIR)/%.d)

SRC := main.c serial.c interrupt.c printf.c pic.c pit.c util.c memory.c buddy.c paging.c slab.c threading.c
OBJ := $(AOBJ) $(SRC:%.c=$(INTERDIR)/%.o)
DEP := $(ADEP) $(SRC:%.c=$(INTERDIR)/%.d)

all: kernel

kernel: $(OBJ) kernel.ld | $(INTERDIR)
	$(LD) $(LFLAGS) -T kernel.ld -o $@ $(OBJ)

$(INTERDIR)/%.o: %.S | $(INTERDIR)
	$(CC) -D__ASM_FILE__ -g -MMD -c $< -o $@

$(INTERDIR)/%.o: %.c | $(INTERDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(INTERDIR):
	mkdir $@

-include $(DEP)

.PHONY: clean
clean:
	rm -f kernel
	rm -rf $(INTERDIR)

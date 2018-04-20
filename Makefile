# Customize

CFLAGS = -march=armv7-m -mthumb -mtune=cortex-m3
LD_SCRIPT = stm32f103x8.ld

# Common

CROSS_COMPILE ?= arm-none-eabi
CC := $(CROSS_COMPILE)-gcc
LD := $(CROSS_COMPILE)-ld
OC := $(CROSS_COMPILE)-objcopy
OD := $(CROSS_COMPILE)-objdump

CFLAGS += -fno-builtin -nostdlib -nostartfiles
CFLAGS += -Wall -Wunused-parameter -Werror -Wno-main #-Wpointer-arith
CFLAGS += -O2

TARGET	= yaboot
SRCS    = $(wildcard *.c)
OBJS	= $(SRCS:.c=.o)

LDFLAGS = -T$(LD_SCRIPT)
ODFLAGS = -Dsx

all: $(TARGET).bin $(TARGET).dump
	@printf "  Section Size(in bytes):\n"
	@awk '/^.text/ || /^.data/ || /^.bss/ {printf("  %s\t\t %8d\n", $$1, strtonum($$3))}' $(TARGET).map
	@wc -c $(TARGET).bin | awk '{printf("  .bin\t\t %8d\n", $$1)}'

$(TARGET).dump: $(TARGET).elf
	$(OD) $(ODFLAGS) $< > $@
$(TARGET).bin: $(TARGET).elf
	$(OC) $(OCFLAGS) -O binary $< $@
$(TARGET).elf : $(OBJS)
	$(LD) -o $@ $^ -Map $(TARGET).map $(LDFLAGS)
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET).bin $(TARGET).dump $(TARGET).elf $(TARGET).map
.PHONY: flash
flash:
	st-flash --reset write $(TARGET).bin 0x08000000

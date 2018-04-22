# Customize

FLASH_ADDR = 0x08000000
LD_SCRIPT = bsp/stm32f103x8.ld
MACH = stm32f1
CFLAGS = -march=armv7-m -mthumb -mtune=cortex-m3

# Common

CROSS_COMPILE ?= arm-none-eabi
CC := $(CROSS_COMPILE)-gcc
LD := $(CROSS_COMPILE)-ld
OC := $(CROSS_COMPILE)-objcopy
OD := $(CROSS_COMPILE)-objdump

CFLAGS += -nostartfiles
CFLAGS += -Wall -Wunused-parameter -Werror -Wno-main #-Wpointer-arith
CFLAGS += -O2
CFLAGS += -D$(MACH)

TARGET	= yaboot
SRCS    = $(wildcard *.c)
OBJS	= $(SRCS:.c=.o)
INCS	= -I./bsp

LDFLAGS = -T$(LD_SCRIPT)
#LDFLAGS += -L$(HOME)/Toolchain/gcc-arm-none-eabi-7-2017-q4-major/arm-none-eabi/lib -lc
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
	#$(LD) -o $@ $^ -Map $(TARGET).map $(LDFLAGS)
	$(CC) $(CFLAGS) $(INCS) -o $@ $^ -Wl,-Map,$(TARGET).map $(LDFLAGS)
.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET).bin $(TARGET).dump $(TARGET).elf $(TARGET).map
.PHONY: flash burn
flash burn:
	st-flash --reset write $(TARGET).bin $(FLASH_ADDR)

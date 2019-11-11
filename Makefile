# Customize

FLASH_ADDR = 0x08000000
LD_SCRIPT = bsp/stm32f103xE.ld
MACH = stm32f1
CFLAGS = -march=armv7-m -mthumb -mtune=cortex-m3

# Common

CROSS_COMPILE ?= arm-none-eabi
CC := $(CROSS_COMPILE)-gcc
LD := $(CROSS_COMPILE)-ld
OC := $(CROSS_COMPILE)-objcopy
OD := $(CROSS_COMPILE)-objdump

CFLAGS += -std=gnu99 -Os \
	  -ffunction-sections -fdata-sections -Wl,--gc-sections \
	  -nostartfiles
CFLAGS += -W -Wall -Wunused-parameter -Wno-main -Wextra -Wformat-nonliteral \
	  -Wpointer-arith -Wbad-function-cast \
	  -Wshadow -Wwrite-strings -Wstrict-aliasing \
	  -Wmissing-format-attribute -Wmissing-include-dirs \
	  -Waggregate-return -Winit-self -Wlogical-op -Wredundant-decls \
	  -Wdouble-promotion -Wfloat-equal -Wformat-overflow
CFLAGS += -Werror -Wno-error=aggregate-return -Wno-error=pedantic
CFLAGS += -D$(MACH) -DDEBUG #-DQUICKBOOT

TARGET	= yaboot
SRCS    = $(wildcard *.c) \
	  tools/tinycrypt/lib/source/aes_encrypt.c \
	  tools/tinycrypt/lib/source/ctr_mode.c \
	  tools/tinycrypt/lib/source/sha256.c \
	  tools/tinycrypt/lib/source/ecc.c \
	  tools/tinycrypt/lib/source/ecc_dsa.c \
	  tools/tinycrypt/lib/source/utils.c
OBJS	= $(SRCS:.c=.o)
INCS	= -Ibsp -Itools \
	  -Itools/tinycrypt/lib/include
CFLAGS += -DCTR=1 #-DCBC=1

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
	rm -f *.o $(TARGET).bin $(TARGET).dump $(TARGET).elf $(TARGET).map $(OBJS)
.PHONY: flash burn
flash burn:
	st-flash --reset write $(TARGET).bin $(FLASH_ADDR)

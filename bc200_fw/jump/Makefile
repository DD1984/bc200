CC=$(CROSS_PREFIX)gcc
OBJCOPY=$(CROSS_PREFIX)objcopy

CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb

LDFLAGS += -T gcc_nrf52.ld
LDFLAGS += -nostartfiles -nodefaultlibs

all:
	$(CC) $(CFLAGS) -c gcc_startup_nrf52840.S
	$(CC) $(CFLAGS) -c jump.c
	$(CC) $(LDFLAGS) jump.o gcc_startup_nrf52840.o -o jump.elf
	$(OBJCOPY) -Obinary jump.elf jump.bin
	$(OBJCOPY) -Oihex jump.elf jump.hex

clean:
	rm -rf *.o *.elf *.hex *.bin

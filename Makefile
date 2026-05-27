CC     = gcc
CFLAGS = -Wall -Wextra -g -O0
LIBS   = -lm

SRC = main.c bootloader.c flash_sim.c crc32.c uart_sim.c
OUT = bootloader_sim

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT) /tmp/stm32_flash.bin /tmp/firmware_update.bin /tmp/boot_update_trigger
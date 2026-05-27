#include "flash_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t g_flash[FLASH_SIZE];
static char    g_filepath[256];
static int     g_initialized = 0;

int flash_sim_init(const char *filepath) {
    memset(g_flash, 0xFF, FLASH_SIZE); // flash erased state = 0xFF
    strncpy(g_filepath, filepath, sizeof(g_filepath) - 1);

    FILE *f = fopen(filepath, "rb");
    if (f) {
        fread(g_flash, 1, FLASH_SIZE, f);
        fclose(f);
        printf("[FLASH] Loaded existing flash image from %s\n", filepath);
    } else {
        printf("[FLASH] New flash image initialized (all 0xFF)\n");
    }
    g_initialized = 1;
    return FLASH_OK;
}

static int addr_to_offset(uint32_t addr) {
    if (addr < BOOTLOADER_BASE) return -1;
    return (int)(addr - BOOTLOADER_BASE);
}

int flash_sim_read(uint32_t addr, uint8_t *buf, size_t len) {
    int offset = addr_to_offset(addr);
    if (offset < 0 || offset + (int)len > FLASH_SIZE) return FLASH_ERR_ADDR;
    memcpy(buf, &g_flash[offset], len);
    return FLASH_OK;
}

int flash_sim_write(uint32_t addr, const uint8_t *buf, size_t len) {
    int offset = addr_to_offset(addr);
    if (offset < 0 || offset + (int)len > FLASH_SIZE) return FLASH_ERR_ADDR;

    // Real flash: can only write 0s into existing 1s (must erase first)
    for (size_t i = 0; i < len; i++) {
        g_flash[offset + i] &= buf[i];
    }

    // Persist to file
    FILE *f = fopen(g_filepath, "wb");
    if (!f) return FLASH_ERR_IO;
    fwrite(g_flash, 1, FLASH_SIZE, f);
    fclose(f);
    printf("[FLASH] Written %zu bytes at 0x%08X\n", len, addr);
    return FLASH_OK;
}

int flash_sim_erase(uint32_t addr, size_t len) {
    int offset = addr_to_offset(addr);
    if (offset < 0 || offset + (int)len > FLASH_SIZE) return FLASH_ERR_ADDR;
    memset(&g_flash[offset], 0xFF, len);

    FILE *f = fopen(g_filepath, "wb");
    if (!f) return FLASH_ERR_IO;
    fwrite(g_flash, 1, FLASH_SIZE, f);
    fclose(f);
    printf("[FLASH] Erased %zu bytes at 0x%08X\n", len, addr);
    return FLASH_OK;
}

void flash_sim_dump(uint32_t addr, size_t len) {
    int offset = addr_to_offset(addr);
    if (offset < 0) return;
    printf("[FLASH] Dump at 0x%08X:\n", addr);
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("  %08X: ", (unsigned)(addr + i));
        printf("%02X ", g_flash[offset + i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

void flash_sim_close(void) {
    g_initialized = 0;
}
#include "crc32.h"

// CRC32 implemented from scratch using polynomial 0xEDB88320
// Same polynomial used in STM32 hardware CRC unit and Ethernet

static uint32_t crc32_table[256];
static int      table_initialized = 0;

static void crc32_init_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    table_initialized = 1;
}

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    if (!table_initialized) crc32_init_table();
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

uint32_t crc32_compute(const uint8_t *data, size_t len) {
    return crc32_update(0, data, len);
}
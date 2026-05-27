#include "bootloader.h"
#include "flash_sim.h"
#include "crc32.h"
#include "uart_sim.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static BootState g_state = BOOT_STATE_INIT;

int bootloader_init(void) {
    printf("[BOOT] Bootloader v1.0 — 0x%08X\n", BOOTLOADER_BASE);
    printf("[BOOT] Flash layout:\n");
    printf("       Bootloader : 0x%08X (%d KB)\n", BOOTLOADER_BASE, BOOTLOADER_SIZE/1024);
    printf("       Metadata   : 0x%08X (%d KB)\n", METADATA_BASE,   METADATA_SIZE/1024);
    printf("       App        : 0x%08X (%d KB)\n", APP_BASE,        APP_SIZE/1024);
    printf("       Staging    : 0x%08X (%d KB)\n", STAGING_BASE,    STAGING_SIZE/1024);
    return BOOT_OK;
}

int bootloader_check_trigger(void) {
    // In simulation: check if a trigger file exists
    // On hardware: read a GPIO pin or check a RAM flag
    FILE *f = fopen("/tmp/boot_update_trigger", "r");
    if (f) {
        fclose(f);
        printf("[BOOT] Update trigger detected\n");
        return 1;
    }
    return 0;
}

int bootloader_verify_app(void) {
    AppMetadata meta;
    int ret = flash_sim_read(METADATA_BASE, (uint8_t*)&meta, sizeof(meta));
    if (ret != FLASH_OK) {
        printf("[BOOT] ERROR: Cannot read metadata\n");
        return BOOT_ERR_NO_APP;
    }

    if (meta.magic != BOOT_MAGIC) {
        printf("[BOOT] ERROR: Invalid magic 0x%08X\n", meta.magic);
        return BOOT_ERR_MAGIC;
    }

    printf("[BOOT] App metadata: version=%u size=%u bytes\n",
           meta.version, meta.size);

    // Read app and compute CRC
    uint8_t *app_buf = malloc(meta.size);
    if (!app_buf) return BOOT_ERR_NO_APP;

    flash_sim_read(APP_BASE, app_buf, meta.size);
    uint32_t computed = crc32_compute(app_buf, meta.size);
    free(app_buf);

    if (computed != meta.crc32) {
        printf("[BOOT] ERROR: CRC mismatch — stored=0x%08X computed=0x%08X\n",
               meta.crc32, computed);
        return BOOT_ERR_CRC;
    }

    printf("[BOOT] CRC OK: 0x%08X\n", computed);
    return BOOT_OK;
}

void bootloader_jump_to_app(void) {
    // In simulation: print what would happen on hardware
    // On STM32: set SP from app vector table, jump to reset handler
    printf("[BOOT] ===================================\n");
    printf("[BOOT] Jumping to application at 0x%08X\n", APP_BASE);
    printf("[BOOT] On STM32 this would:\n");
    printf("       1. Read stack pointer from 0x%08X\n", APP_BASE);
    printf("       2. Read reset handler from  0x%08X\n", APP_BASE + 4);
    printf("       3. Relocate vector table to 0x%08X\n", APP_BASE);
    printf("       4. Set MSP and branch to reset handler\n");
    printf("[BOOT] ===================================\n");
    printf("[APP]  Motor control firmware running...\n");
}

void bootloader_run(void) {
    g_state = BOOT_STATE_INIT;
    bootloader_init();

    while (1) {
        switch (g_state) {

        case BOOT_STATE_INIT:
            g_state = BOOT_STATE_CHECK_TRIGGER;
            break;

        case BOOT_STATE_CHECK_TRIGGER:
            if (bootloader_check_trigger()) {
                g_state = BOOT_STATE_UPDATE_MODE;
            } else {
                g_state = BOOT_STATE_VERIFY_APP;
            }
            break;

        case BOOT_STATE_UPDATE_MODE:
            printf("[BOOT] Entering firmware update mode\n");
            uart_receive_firmware();
            g_state = BOOT_STATE_VERIFY_APP;
            break;

        case BOOT_STATE_VERIFY_APP:
            printf("[BOOT] Verifying application image...\n");
            if (bootloader_verify_app() == BOOT_OK) {
                g_state = BOOT_STATE_JUMP_TO_APP;
            } else {
                g_state = BOOT_STATE_ERROR;
            }
            break;

        case BOOT_STATE_JUMP_TO_APP:
            bootloader_jump_to_app();
            return;

        case BOOT_STATE_ERROR:
            printf("[BOOT] FAULT: Staying in bootloader — waiting for update\n");
            uart_receive_firmware();
            g_state = BOOT_STATE_VERIFY_APP;
            break;
        }
    }
}
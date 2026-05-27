#include "bootloader.h"
#include "flash_sim.h"
#include <stdio.h>

int main(void) {
    printf("========================================\n");
    printf("  STM32 Bootloader Simulator\n");
    printf("  Target: ARM Cortex-M4 @ 0x08000000\n");
    printf("========================================\n\n");

    // Initialize simulated flash (persists to file)
    flash_sim_init("/tmp/stm32_flash.bin");

    // Run bootloader state machine
    bootloader_run();

    flash_sim_close();
    return 0;
}
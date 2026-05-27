#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

#define BOOT_MAGIC          0xDEADBEEF
#define BOOT_OK             0
#define BOOT_ERR_NO_APP    -1
#define BOOT_ERR_CRC       -2
#define BOOT_ERR_MAGIC     -3

typedef enum {
    BOOT_STATE_INIT,
    BOOT_STATE_CHECK_TRIGGER,
    BOOT_STATE_UPDATE_MODE,
    BOOT_STATE_VERIFY_APP,
    BOOT_STATE_JUMP_TO_APP,
    BOOT_STATE_ERROR
} BootState;

int  bootloader_init(void);
void bootloader_run(void);
int  bootloader_verify_app(void);
void bootloader_jump_to_app(void);
int  bootloader_check_trigger(void);

#endif
#ifndef FLASH_SIM_H
#define FLASH_SIM_H

#include <stdint.h>
#include <stddef.h>

#define FLASH_SIZE          (512 * 1024)    // 512 KB total
#define BOOTLOADER_BASE     0x08000000
#define BOOTLOADER_SIZE     (32 * 1024)     // 32 KB
#define METADATA_BASE       0x08008000
#define METADATA_SIZE       (4 * 1024)      // 4 KB
#define APP_BASE            0x08009000
#define APP_SIZE            (128 * 1024)    // 128 KB
#define STAGING_BASE        0x08029000
#define STAGING_SIZE        (128 * 1024)    // 128 KB

#define FLASH_OK            0
#define FLASH_ERR_ADDR     -1
#define FLASH_ERR_SIZE     -2
#define FLASH_ERR_IO       -3

typedef struct {
    uint32_t magic;         // 0xDEADBEEF if valid
    uint32_t version;       // firmware version
    uint32_t size;          // app size in bytes
    uint32_t crc32;         // CRC32 of app image
    uint8_t  reserved[16];
} AppMetadata;

int     flash_sim_init(const char *filepath);
int     flash_sim_read(uint32_t addr, uint8_t *buf, size_t len);
int     flash_sim_write(uint32_t addr, const uint8_t *buf, size_t len);
int     flash_sim_erase(uint32_t addr, size_t len);
void    flash_sim_dump(uint32_t addr, size_t len);
void    flash_sim_close(void);

#endif
#include "uart_sim.h"
#include "bootloader.h"
#include "flash_sim.h"
#include "crc32.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// CRC16 for packet integrity
static uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else         crc >>= 1;
        }
    }
    return crc;
}

void uart_send_ack(uint16_t packet_num) {
    printf("[UART] ACK  packet %d\n", packet_num);
}

void uart_send_nack(uint16_t packet_num) {
    printf("[UART] NACK packet %d — CRC error, requesting retransmit\n", packet_num);
}

int uart_receive_firmware(void) {
    printf("[UART] Waiting for firmware image...\n");
    printf("[UART] Protocol: magic=0xAA55 | seq | len | crc16 | data[128]\n");

    // Read firmware packets from a binary file (simulates UART stream)
    FILE *f = fopen("/tmp/firmware_update.bin", "rb");
    if (!f) {
        printf("[UART] ERROR: No firmware file at /tmp/firmware_update.bin\n");
        printf("[UART] Run host_updater.py first to generate firmware\n");
        return -1;
    }

    // Erase staging area first
    flash_sim_erase(STAGING_BASE, STAGING_SIZE);

    uint32_t write_addr  = STAGING_BASE;
    uint32_t total_bytes = 0;
    uint16_t expected    = 0;
    UARTPacket pkt;

    while (fread(&pkt, sizeof(pkt), 1, f) == 1) {
        // Validate magic
        if (pkt.magic != PACKET_MAGIC) {
            printf("[UART] ERROR: Bad magic 0x%04X at packet %d\n",
                   pkt.magic, expected);
            fclose(f);
            return -1;
        }

        // Validate sequence
        if (pkt.packet_num != expected) {
            printf("[UART] ERROR: Out of sequence — expected %d got %d\n",
                   expected, pkt.packet_num);
            uart_send_nack(pkt.packet_num);
            fclose(f);
            return -1;
        }

        // Validate CRC16
        uint16_t computed = crc16(pkt.data, pkt.data_len);
        if (computed != pkt.crc16) {
            uart_send_nack(pkt.packet_num);
            fclose(f);
            return -1;
        }

        // Write to staging flash
        flash_sim_write(write_addr, pkt.data, pkt.data_len);
        write_addr  += pkt.data_len;
        total_bytes += pkt.data_len;

        uart_send_ack(pkt.packet_num);
        expected++;
    }

    fclose(f);
    printf("[UART] Received %u bytes in %d packets\n", total_bytes, expected);

    // Copy staging → app + write metadata
    printf("[BOOT] Copying staging → app sector\n");
    uint8_t *buf = malloc(total_bytes);
    flash_sim_read(STAGING_BASE, buf, total_bytes);
    flash_sim_erase(APP_BASE, APP_SIZE);
    flash_sim_write(APP_BASE, buf, total_bytes);

    // Write metadata
    AppMetadata meta;
    meta.magic   = BOOT_MAGIC;
    meta.version = 1;
    meta.size    = total_bytes;
    meta.crc32   = crc32_compute(buf, total_bytes);
    free(buf);

    flash_sim_erase(METADATA_BASE, METADATA_SIZE);
    flash_sim_write(METADATA_BASE, (uint8_t*)&meta, sizeof(meta));

    printf("[BOOT] Metadata written: version=%u size=%u crc32=0x%08X\n",
           meta.version, meta.size, meta.crc32);
    return 0;
}
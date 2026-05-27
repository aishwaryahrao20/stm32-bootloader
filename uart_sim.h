#ifndef UART_SIM_H
#define UART_SIM_H

#include <stdint.h>

#define PACKET_MAGIC        0xAA55
#define PACKET_DATA_SIZE    128
#define PACKET_HEADER_SIZE  8

typedef struct __attribute__((packed)) {
    uint16_t magic;         // 0xAA55
    uint16_t packet_num;    // sequence number
    uint16_t data_len;      // bytes in this packet
    uint16_t crc16;         // CRC16 of data only
    uint8_t  data[PACKET_DATA_SIZE];
} UARTPacket;

int  uart_receive_firmware(void);
void uart_send_ack(uint16_t packet_num);
void uart_send_nack(uint16_t packet_num);

#endif
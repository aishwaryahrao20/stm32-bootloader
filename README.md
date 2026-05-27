# STM32 Bootloader & Firmware Update System

Bare-metal bootloader implemented in C targeting ARM Cortex-M4, featuring a fault-tolerant boot state machine, CRC32 image verification, sector-level flash programming, and a chunked UART firmware update protocol. Architected to run as a full simulation on macOS/Linux with direct STM32 hardware portability.

---

## Boot State Machine

```
INIT → CHECK_TRIGGER → UPDATE_MODE → VERIFY_APP → JUMP_TO_APP
                    ↘ VERIFY_APP ↗               ↘ ERROR → UPDATE_MODE
```

| State | Action |
|-------|--------|
| INIT | Print flash layout, initialize peripherals |
| CHECK_TRIGGER | GPIO pin or flag — update vs normal boot |
| UPDATE_MODE | Receive firmware over UART, flash to staging |
| VERIFY_APP | CRC32 check of app image against metadata |
| JUMP_TO_APP | Set SP, relocate vector table, branch to reset handler |
| ERROR | CRC fail — stay in bootloader, await new image |

---

## Flash Memory Layout

```
0x08000000  ┌─────────────────┐
            │   Bootloader    │  32 KB  write-protected
0x08008000  ├─────────────────┤
            │   App Metadata  │   4 KB  magic · version · size · CRC32
0x08009000  ├─────────────────┤
            │   Application   │ 128 KB  motor control firmware
0x08029000  ├─────────────────┤
            │   Staging Area  │ 128 KB  incoming firmware buffer
0x08049000  └─────────────────┘
```

---

## UART Firmware Update Protocol

| Field | Size | Value |
|-------|------|-------|
| Magic | 2 bytes | 0xAA55 |
| Sequence | 2 bytes | Packet number |
| Length | 2 bytes | Data bytes in packet |
| CRC16 | 2 bytes | CRC16 of data only |
| Data | 128 bytes | Firmware chunk |

- Host sends firmware in 128-byte chunks
- Bootloader ACKs each packet on CRC pass
- Bootloader NACKs and requests retransmit on CRC fail
- After all packets received: staging → app copy → metadata write → verify

---

## CRC32 Implementation

Implemented from scratch using polynomial `0xEDB88320` — identical to the STM32 hardware CRC unit and Ethernet standard. Used for both per-packet integrity (CRC16) and full image verification (CRC32) before every boot.

---

## Results

| Test | Result |
|------|--------|
| Blank flash boot | Stays in bootloader — waits for update |
| Firmware update (4 packets, 512 bytes) | All packets ACK'd, CRC32=0x4FACB929 |
| Normal boot after update | CRC verified, jump to 0x08009000 |
| Corrupt image boot | CRC mismatch detected, stays in bootloader |

---

## File Structure

```
stm32_bootloader/
├── bootloader.h/c     # Boot state machine + jump logic
├── flash_sim.h/c      # 512KB flash simulation (file-backed)
├── crc32.h/c          # CRC32 from scratch (poly 0xEDB88320)
├── uart_sim.h/c       # Chunked UART packet receiver
├── host_updater.py    # Python firmware packager + sender
├── main.c             # Entry point
└── Makefile
```

---

## Quick Start

```bash
# Build
gcc -Wall -Wextra -g -O0 main.c bootloader.c flash_sim.c crc32.c uart_sim.c -o bootloader_sim -lm

# Normal boot (blank flash — stays in bootloader)
./bootloader_sim

# Firmware update cycle
python3 host_updater.py                  # generate firmware packets
touch /tmp/boot_update_trigger           # set update trigger
./bootloader_sim                         # run update + boot

# Normal boot (firmware already flashed)
rm /tmp/boot_update_trigger
./bootloader_sim
```

---

## Hardware Porting (STM32F446RE)

| Sim | STM32 HAL |
|-----|-----------|
| `flash_sim_write()` | `HAL_FLASH_Program()` |
| `flash_sim_erase()` | `HAL_FLASHEx_Erase()` |
| `fopen/fread` UART | `HAL_UART_Receive_IT()` |
| Trigger file check | `HAL_GPIO_ReadPin()` |
| `bootloader_jump_to_app()` | Set MSP + branch via inline assembly |

Jump implementation on STM32:
```c
void jump_to_app(uint32_t app_base) {
    uint32_t sp  = *(volatile uint32_t*)(app_base);
    uint32_t pc  = *(volatile uint32_t*)(app_base + 4);
    __set_MSP(sp);
    ((void(*)(void))pc)();
}
```

---

## Skills Demonstrated

- Bare-metal C firmware — no OS, no HAL abstractions
- ARM Cortex-M4 boot sequence — stack pointer, vector table, reset handler
- Flash memory management — sector erase, write, CRC verify
- Binary protocol design — framed packets, sequence numbers, CRC integrity
- Fault-tolerant state machine — safe fallback on corruption
- Python host tooling — firmware packaging and binary transfer

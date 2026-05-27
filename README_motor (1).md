# Motor Control Firmware Simulator

Portable closed-loop motor control firmware written in C, architected with a Hardware Abstraction Layer (HAL) that enables identical application logic to run on a macOS/Linux simulation environment and bare-metal STM32 (ARM Cortex-M4) without any changes above the HAL boundary.

---

## Architecture

```
+--------------------------------------------------+
|                 Application Layer                |
|  main.c · rtos_sim.c · pid.c · motor_model.c    |
+--------------------------------------------------+
|            HAL Interface (hal.h)                 |
|  hal_uart_send · hal_i2c_read · hal_spi_write    |
|  hal_pwm_set_duty · hal_encoder_get_ticks        |
+--------------------------------------------------+
|        Sim Backend          |   STM32 Backend     |
|        hal_sim.c            |   hal_stm32.c       |
|  printf · pthreads · math   |  USART·I2C·SPI·CAN |
+-----------------------------+---------------------+
```

---

## Protocol Stack

| Protocol | Role | Simulated via |
|----------|------|---------------|
| UART | Real-time telemetry stream | printf to terminal |
| SPI | Motor driver config (DRV8323) | Logged transaction |
| I2C | Thermal monitoring (TMP102 @ 0x48) | Software motor model |
| CAN | Speed setpoint commands | Simulated CAN frame parser |
| PWM | Motor drive signal | Motor model duty input |

---

## RTOS Task Structure

| Task | Period | Responsibility |
|------|--------|----------------|
| encoder | 1 ms | Read motor speed, update model |
| pid | 5 ms | Compute PID output, set PWM duty |
| can | 10 ms | Parse CAN frames, update setpoint |
| telemetry | 50 ms | Stream UART log, poll I2C + SPI |

Shared state protected by POSIX mutex — mirrors FreeRTOS `xSemaphoreTake` pattern.

---

## PID Controller

| Parameter | Value |
|-----------|-------|
| Kp | 0.8 |
| Ki | 0.3 |
| Kd | 0.02 |
| Output range | 0 – 100% PWM duty |
| Anti-windup | Integral clamped to output range |

---

## Results

| Metric | Value |
|--------|-------|
| Setpoint | 1500 RPM |
| Steady-state RPM | 1476 RPM |
| Steady-state error | 1.6% |
| Rise time (0 to 90%) | < 500 ms |
| Thermal steady-state | ~41°C at 50% load |
| PWM duty at steady state | ~49% |

---

## File Structure

```
motor_firmware_sim/
├── hal.h               # HAL interface — protocol contract
├── hal_sim.c           # Simulation backend (Mac/Linux)
├── motor_model.h/c     # First-order motor + thermal model
├── pid.h/c             # PID controller with anti-windup
├── rtos_sim.h/c        # POSIX-thread based RTOS scheduler
├── main.c              # Task definitions + system init
├── plot_response.py    # Step response parser + plotter
├── step_response.log   # Sample telemetry log
└── Makefile            # Build system
```

---

## Quick Start

**Requirements:** GCC, make, pthreads, Python 3 + matplotlib

```bash
# Build
make

# Run simulator
./motor_sim

# Log and plot step response
./motor_sim 2>/dev/null | grep "UART" > step_response.log & sleep 10 && kill %1
python3 plot_response.py
```

---

## Hardware Porting (STM32F446RE)

The HAL boundary makes porting straightforward:

1. Replace `hal_sim.c` with `hal_stm32.c`
2. Implement each HAL function using STM32 HAL calls:
   - `hal_uart_send()` → `HAL_UART_Transmit()`
   - `hal_i2c_read()` → `HAL_I2C_Mem_Read()`
   - `hal_spi_write()` → `HAL_SPI_TransmitReceive()`
   - `hal_pwm_set_duty()` → `__HAL_TIM_SET_COMPARE()`
   - `hal_encoder_get_ticks()` → `TIM_ENCODERMODE`
3. Replace `rtos_sim.c` with FreeRTOS task definitions
4. All PID, motor model, and protocol logic remain unchanged

**Target hardware:** NUCLEO-F446RE + DC gearmotor + L298N + TMP102 + SN65HVD230

---

## Skills Demonstrated

- Hardware Abstraction Layer (HAL) design for sim-to-hardware portability
- RTOS task scheduling with mutex-protected shared state
- Closed-loop PID control with anti-windup
- Multi-protocol firmware stack: UART · SPI · I2C · CAN · PWM
- C firmware architecture on ARM Cortex-M4 target
- Python telemetry analysis and step response plotting

# STM32 temperature controlled fan

## Hardware and pin audit

The CubeMX project targets **STM32F411CEU6** (UFQFPN48). After the latest reconnection, OpenOCD read SWD device ID **0x431**, confirming an STM32F411 family target ([ST RM0383](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)). An earlier connection returned `0x433` (STM32F401xD/xE), so always verify the ID before flashing. The debug ID does not identify the exact package or printed part suffix.

| Function | MCU pin | Verified mapping | Current electrical/configuration state |
| --- | --- | --- | --- |
| TMP117 SDA | PB3 | I2C2 SDA, AF9 | Open drain, no internal pull-up |
| TMP117 SCL | PB10 | I2C2 SCL, AF4 | Open drain, no internal pull-up |
| TMP117 ALERT | PB2 | GPIO / EXTI2 | Rising edge selected, no pull-up; interrupt not used |
| ESP8266 TX from STM32 | PB6 | USART1 TX, AF7 | Push-pull alternate function; USART1 not initialized yet |
| ESP8266 RX into STM32 | PB7 | USART1 RX, AF7 | Alternate function, no pull; USART1 not initialized yet |
| Fan MOSFET gate | PA0 | TIM2 channel 1, AF1 | Push-pull alternate function; PWM not started yet |

The requested I2C2 wiring **is valid**. No wire move is required. PB3 also carries SWO, so SWO trace must not be enabled while it is used for SDA; ordinary SWD on PA13/PA14 remains available. Alternate functions are from [ST DS10314, Table 9](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf).

TMP117 ADD0 is grounded, giving a 7-bit address of **0x48**. The driver passes **0x90** to the STM32 HAL I2C APIs, which require the address shifted left. I2C2 is configured at **100 kHz** from the 16 MHz HSI clock. The TMP117 driver checks device ID bits 11:0 for **0x117**, reads the 16-bit MSB-first result, and treats reset value **0x8000** as unready. Temperature is `signed_raw * 0.0078125` °C. These values follow the [TI TMP117 datasheet](https://www.ti.com/lit/ds/symlink/tmp117.pdf).

Before sensor bring-up, verify external SDA and SCL pull-ups to a voltage safe for both devices (normally 3.3 V), sensor supply and common ground, and a 0.1 µF capacitor near TMP117 V+. ALERT is open drain and needs a pull-up when it is used. The existing PB2 rising-edge setting does not match the TMP117 default active-low ALERT; it is intentionally unused in this stage.

The fan is intended to be switched through an AX3400 N-channel MOSFET, not powered by PA0. Confirm its gate pull-down and any gate resistor, fan supply, and flyback/freewheel protection before fan tests. The ESP8266 uses a separate supply with common ground.

## Current bring-up stage

The bring-up firmware reads the TMP117 device ID, configuration, and temperature registers directly at startup, then retries or samples every 500 ms. It does not wait for PB2/ALERT or the configuration register's `Data_Ready` bit. A 10 ms delay after MCU initialization precedes the first read. No UART, automatic fan control, Wi-Fi, or web interface is active yet.

Watch these globals in a debugger: `tmp117_probe_status`, `tmp117_id_status`, `tmp117_config_status`, `tmp117_temperature_status`, `tmp117_device_id`, `tmp117_configuration`, `tmp117_raw_temperature`, `tmp117_temperature_c`, `tmp117_last_success_tick`, `tmp117_error_count`, `tmp117_i2c_error`, `tmp117_address_scan_done`, and `tmp117_possible_address_ack_mask`. A successful hardware reading has HAL status `HAL_OK` (0), device ID with low 12 bits `0x117`, and a plausible Celsius value. `HAL_BUSY` for temperature can mean the first conversion is not complete. In the address mask, bits 0–3 correspond to addresses `0x48`–`0x4B`.

## Build and flash

The repository contains a CubeMX generated IAR project in `EWARM/`. An IAR compiler is not installed on the current Mac. A GCC build is available using the IAR project's source list:

```sh
python3 gcc/build.py
```

This requires `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, and `arm-none-eabi-size`. It creates `firmware-build/fan.elf` and `firmware-build/fan.bin`. The GCC build is freestanding and includes a small runtime for this C-only firmware; it does not include a general-purpose C standard library.

After checking the target ID, the bring-up image was programmed and verified with:

```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c 'program firmware-build/fan.elf verify reset exit'
```

The ST-Link is visible as USB VID 0483, PID 3748. OpenOCD reached an STM32F411 target with debug ID `0x10006431` and 512 KB Flash. Before programming, the full prior Flash contents were saved to `firmware-build/target-before-tmp117.bin` (SHA-256 `ddc5b512da6a81f2319b95482d13be520522c08b533bacd6d12ffc8c8decc606`). The direct-read TMP117 image was flashed and verified. After the no-ACK test, the original first 16 KB Flash sector was restored from `firmware-build/target-original-sector0.bin` and verified; the rest of Flash was not changed by the test. The restored firmware's functional behavior has not been evaluated.

OpenOCD repeatedly reported target reference voltage around **3.75 V**; the [STM32F411 datasheet](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf) specifies VDD operation only up to **3.6 V**. The user indicated the supply is satisfactory but has no means to measure it. Actual VDD remains unverified.

## Hardware bring-up result

The STM32 ran the direct-read TMP117 diagnostic firmware, but device ID, configuration, and temperature reads all returned `HAL_ERROR` (1). The HAL I2C error was `HAL_I2C_ERROR_AF` (1), meaning no ACK. A one-time scan of all TMP117 ADD0 addresses `0x48`–`0x4B` completed with `tmp117_possible_address_ack_mask = 0`. I2C2 was enabled at 100 kHz; PB3 and PB10 were configured for open-drain alternate functions and both read high at idle. This test did not use ALERT or `Data_Ready`. The sensor module's V+, ground, SDA, SCL, ADD0, and external pull-ups must be checked physically. No physical temperature reading has been observed.

## Configuration still pending

- USART1 on PB6/PB7 has pin AF7 selected but no USART initialization or baud rate. The intended starting point is 115200 8N1 after ESP8266 firmware is identified.
- TIM2 channel 1 is configured with 16 MHz timer clock, prescaler 83, ARR 999, and CCR1 500. This would produce about **190.48 Hz** at 50% duty if started; PWM is not started. Frequency and gate polarity need physical fan testing before use.
- TMP117 ALERT is not used. Its pull-up and interrupt edge must be corrected before enabling it.
- Fan control, sensor-failure full-speed fail-safe, ESP8266 protocol, open Wi-Fi SSID `CMF by Nothing Phone 1_5733` (no password), and web interface have not yet been implemented. Those stages depend on verified sensor and fan bring-up.

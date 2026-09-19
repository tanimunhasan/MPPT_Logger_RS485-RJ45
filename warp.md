# EPEVER MPPT Logger

## Purpose

This firmware runs on a LilyGO T-CAN485 ESP32 board. It communicates with an EPEVER Tracer MPPT solar charge controller over RS-485/Modbus RTU, reads PV and battery data, and stores samples in monthly CSV files on a microSD card.

The project uses PlatformIO, the Arduino framework, and the `4-20ma/ModbusMaster` library.

## Quick Start

1. Insert a FAT/FAT32-formatted microSD card.
2. Connect the T-CAN485 RS-485 interface to the EPEVER controller.
3. Flash the firmware.
4. Open the serial terminal at `115200` baud.
5. On the first boot, configure and verify the controller with `WC`.
6. Use `R=2` to test live reads and SD logging.
7. Use `R=3` for low-power deployed logging.

## Hardware

### T-CAN485 pin assignment

The project currently uses these pins:

- RS-485 UART TX: GPIO22
- RS-485 UART RX: GPIO21
- RS-485 callback/control: GPIO17
- RS-485 enable: GPIO19
- RS-485 5 V booster enable: GPIO16
- WS2812 LED data: GPIO4
- microSD MISO: GPIO2
- microSD MOSI: GPIO15
- microSD SCLK: GPIO14
- microSD chip select: GPIO13

GPIO19 is the active RS-485 enable setting in `src/user_config.h`. Some T-CAN485 revisions use GPIO9 instead. If Modbus communication is silent on a different board revision, test GPIO9 only after confirming the wiring.

### EPEVER RS-485 wiring

The wiring notes currently used by this project are:

- EPEVER RJ45 pin 4 / orange wire: RS-485 B
- EPEVER RJ45 pin 5 / yellow wire: RS-485 A
- EPEVER RJ45 pin 7 / blue wire: GND

Connect only the intended RS-485 A, B, and ground lines. Do not connect controller power pins to ESP32 GPIO pins.

If communication fails:

- verify A-to-A and B-to-B wiring;
- confirm the ground connection;
- disconnect other Modbus masters such as a PC program, USB-RS485 adapter, or display while testing;
- verify the controller address and baud rate;
- check the board-specific RS-485 enable GPIO.

## Modbus Connection

The default settings in `src/user_config.h` are:

- Modbus slave address: `1`
- Baud rate: `115200`
- Serial format: `8N1`
- Retry count: `3`
- Delay between retries: `60 ms`

The firmware uses Modbus RTU read-input-register (`0x04`), read-holding-register (`0x03`), write-single-register (`0x06`), and write-multiple-register (`0x10`) operations.

`0xE2` is the ModbusMaster response-timeout error. It means the ESP32 did not receive a complete response from the controller. It is not a battery-configuration error.

## EPEVER Register Map Used by the Firmware

### Live input registers

- `0x3100`: PV voltage, scaled by 0.01 V
- `0x3101`: PV current, scaled by 0.01 A
- `0x3102`–`0x3103`: PV power, 32-bit low/high words, scaled by 0.01 W
- `0x3104`: battery voltage, scaled by 0.01 V
- `0x3105`: charge current, scaled by 0.01 A
- `0x3106`–`0x3107`: charge power, 32-bit low/high words, scaled by 0.01 W
- `0x311A`: battery state of charge, percent
- `0x3200`: battery status raw word
- `0x3201`: charging status raw word

### Generated-energy statistics

The controller reports energy in 0.01 kWh units. The firmware converts each count to 10 Wh.

- `0x330C`–`0x330D`: generated energy today
- `0x330E`–`0x330F`: generated energy this month
- `0x3310`–`0x3311`: generated energy this year
- `0x3312`–`0x3313`: total generated energy

### Battery configuration holding registers

- `0x9000`: battery type
- `0x9001`: battery capacity in Ah
- `0x9002`: temperature compensation
- `0x9003`: high-voltage disconnect
- `0x9004`: charging limit
- `0x9005`: over-voltage reconnect
- `0x9006`: equalization voltage
- `0x9007`: boost/bulk charging voltage
- `0x9008`: float voltage
- `0x9009`: boost/bulk reconnect voltage
- `0x900A`: low-voltage reconnect
- `0x900B`: undervoltage recovery
- `0x900C`: undervoltage warning
- `0x900D`: low-voltage disconnect
- `0x900E`: discharge limit
- `0x9013`–`0x9015`: EPEVER real-time clock
- `0x906B`: equalization duration in minutes
- `0x906C`: boost duration in minutes

## Serial Console

Use a serial monitor at `115200` baud. The console echoes typed characters.

Enter `?` to print the command menu.

### Status and readings

- `S`: print firmware, reset/wake reason, runtime mode, saved mode, buffer count, and error counters.
- `SC`: print the local configuration stored in ESP32 RAM/NVS.
- `RC`: read and print the EPEVER configuration.
- `RD`: read and print one live MPPT sample.
- `RE`: read and print generated-energy statistics.
- `RT`: read the EPEVER real-time clock.
- `ST=YYYY-MM-DD HH:MM:SS`: set and read back the EPEVER real-time clock.

### SD log inspection

- `LOG`: print the current month CSV selected from the last valid MPPT sample time.
- `LOG=YYYY-MM`: print a specific monthly CSV file.

Examples:

```text
LOG
LOG=2026-09
```

### Local configuration commands

`SET` changes the local configuration only. It does not write the EPEVER controller until `WC` is used.

```text
SET TYPE=0
SET CAP=70
SET TEMPCOMP=0
SET HVD=14.60
SET CLV=14.40
SET OVR=14.40
SET EQ=14.20
SET BOOST=14.40
SET FLOAT=13.80
SET BRV=13.20
SET LVR=12.60
SET UVR=12.20
SET UVW=12.00
SET LVD=11.10
SET DLV=11.00
SET EQMIN=0
SET BOOSTMIN=0
SET INTERVAL=300
```

Use the voltage, duration, capacity, and battery-profile values specified by the battery manufacturer and BMS. The values above are examples only.

Supported local keys:

- `TYPE`: battery type code
- `CAP` or `CAPACITY`: battery capacity in Ah
- `TEMPCOMP`: temperature compensation
- `HVD`: high-voltage disconnect
- `CLV`: charging limit voltage
- `OVR`: over-voltage reconnect
- `EQ`: equalization voltage
- `BOOST`: boost/bulk charging voltage
- `FLOAT`: float charging voltage
- `BRV`: boost/bulk reconnect voltage
- `LVR`: low-voltage reconnect
- `UVR`: undervoltage recovery
- `UVW`: undervoltage warning
- `LVD`: low-voltage disconnect
- `DLV`: discharge limit
- `EQMIN`: equalization duration in minutes
- `BOOSTMIN`: boost duration in minutes
- `INTERVAL`: NORMAL-mode sample interval in seconds, from 10 to 86400

The current firmware writes non-positive `TEMPCOMP` values as `0`. Use `SET TEMPCOMP=0` when zero temperature compensation is required; it does not reproduce a negative value displayed by some controller applications.

### Battery type

The traditional EPEVER type codes are:

- `0`: USER-defined profile
- `1`: sealed lead-acid
- `2`: GEL lead-acid
- `3`: flooded lead-acid

Use `SET TYPE=0` when entering custom voltage thresholds. It means the controller uses the values sent by the firmware instead of one of its built-in lead-acid profiles.

The SolarGuardian Li-ion Battery Protect setting is not currently exposed as a console command. Configure it separately in the controller software if required by the battery/BMS.

### Saving and applying a configuration

- `SAVE`: save the current local configuration to ESP32 NVS only.
- `WC`: validate the local configuration, write it to EPEVER, read it back, verify it, and save the verified local configuration to ESP32 NVS.
- `VC`: verify that EPEVER matches the local configuration.
- `FACTORY`: erase the local NVS configuration and restart.

Recommended commissioning procedure:

1. Enter CONFIG mode with `R=1`.
2. Read the controller with `RC`.
3. Enter all required local values using `SET`.
4. Check the local values using `SC`.
5. Run `WC`.
6. Confirm `Configuration VERIFIED and saved.`
7. Run `R=2` to test reads and the SD card.
8. Run `R=3` after the MPPT configuration has been verified.

`R=3` is rejected if the EPEVER configuration is not verified or local configuration changes have not been saved.

## Operating Modes

### `R=1` — CONFIG

CONFIG mode remains awake and processes console commands. It is the normal commissioning and maintenance mode.

### `R=2` — TEST

TEST mode remains awake, reads the MPPT every 10 seconds, prints each sample, and sends every valid sample through the same RTC buffer and microSD flush path used by NORMAL mode.

The SD test flow is:

1. Enter `R=2`.
2. Confirm a live `MPPT SAMPLE` shows expected PV and battery data.
3. Watch the buffer counter:

```text
TEST samples buffered: 1/6
TEST samples buffered: 2/6
```

4. On the sixth valid sample, the firmware writes the batch to SD:

```text
TEST SD flush: PASS
```

5. Print the generated file with `LOG` or `LOG=YYYY-MM`.
6. Return to CONFIG mode with `R=1`.

If Modbus fails, TEST mode prints `TEST read failed. Modbus error 0xE2` and does not store a sample. If SD storage fails, it prints `TEST SD store: FAIL`.

### `R=3` — NORMAL

NORMAL is the low-power deployed logger.

1. It reads one MPPT sample immediately.
2. It stores the sample in RTC RAM.
3. It enters ESP32 deep sleep.
4. It wakes after the saved interval, normally 300 seconds.
5. It repeats until six valid samples are buffered.
6. It initializes the SD card, appends the six rows, powers down SD/SPI, and resumes deep sleep.

With the default 300-second interval, the first SD write occurs about 25 minutes after entering `R=3`, because the first sample occurs immediately. Later flushes occur every 30 minutes.

Timer wakes are intentionally serial-silent when `NORMAL_WAKE_SERIAL_ENABLE` is `0`. ESP32 ROM boot text such as `DEEPSLEEP_RESET` is normal and indicates the timer wake cycle is occurring.

To enter CONFIG after deploying NORMAL mode:

1. Wait until a pending batch has flushed if possible.
2. Press reset or power-cycle the board.
3. During the 10-second cold-boot window, type `?`.

Avoid resetting before a pending batch flushes: samples retained only in RTC RAM may be lost on a cold reset or power interruption.

### `R=4` — HIBERNATE

HIBERNATE flushes pending samples, then enters deep sleep without a timer wake source. Reset or power-cycle the board to resume.

### `R=9` — RESET

RESET restarts the ESP32.

## SD Logging

### File name and format

The EPEVER RTC date chooses the monthly file name:

```text
/YYYY-MM.csv
```

Example:

```text
/2026-09.csv
```

The header is written only when a monthly file is first created:

```text
DateTime,PV_V,PV_A,PV_W,Battery_V,Charge_A,Charge_W,SOC_percent,BatteryStatus,ChargingStatus,EnergyToday_Wh,EnergyMonth_Wh,EnergyYear_Wh,EnergyTotal_Wh
```

Each row includes the EPEVER timestamp, PV electrical values, battery/charge values, raw status words, and energy counters.

### Actual PV input test

Use this test when the PV panel has real sunlight and is connected through the MPPT controller.

1. Safely eject the microSD card from the PC, then insert it into the powered-off or stable T-CAN485 board.
2. Confirm the battery and PV wiring to the EPEVER controller are correct.
3. Enter CONFIG mode with `R=1`.
4. Run a manual live read:

```text
RD
```

5. Confirm that `PV voltage`, `PV current`, and `PV power` are above zero. If they are all `0.00`, the controller is not receiving usable PV input at that time.
6. Start the SD test:

```text
R=2
```

7. Wait for six valid samples, approximately one minute.
8. Confirm:

```text
TEST SD flush: PASS
```

9. View the file from the console:

```text
LOG
```

10. Or safely eject the card and inspect the current monthly CSV on a PC.

Expected solar-data evidence:

- `PV_V` is greater than zero when panel voltage is present.
- `PV_A` is greater than zero when charge current is flowing.
- `PV_W` is greater than zero when the controller is receiving solar power.
- The timestamp advances by 10 seconds in TEST mode.

After successful TEST validation, use `R=3` for deployed logging.

## Deep Sleep and Retained Data

The sample buffer, buffered count, wake count, and error counters use ESP32 RTC memory. RTC memory survives timer deep-sleep wakes but does not survive every reset or power interruption.

The SD card is not powered or initialized for every sample. It is initialized only when a batch must be written, then deinitialized to reduce energy use.

The local configuration is stored separately in ESP32 NVS with a magic value, schema version, and CRC32 check.

## Troubleshooting

### `RT`, `RD`, or TEST mode reports `0xE2`

The firmware timed out waiting for the EPEVER controller. Check the RS-485 wiring, controller address, baud rate, ground, RS-485 enable GPIO, and competing Modbus masters.

### `TEST SD store: FAIL`

Check that the microSD card is fully inserted, formatted as FAT/FAT32, and compatible with the board. Check the SD pin mapping and card condition.

### `LOG failed: requested monthly SD log is unavailable`

The requested file does not exist, the card is absent, or the card cannot be initialized. Run TEST mode until `TEST SD flush: PASS`, then try `LOG` again.

### CSV has `0.00` PV values

SD logging may still be correct. It means the EPEVER reported no PV voltage/current/power during that sample. Verify the PV panel connection, sunlight, controller PV status, and read `RD` again.

### Console commands do not work in NORMAL mode

This is expected. NORMAL mode deep-sleeps and keeps timer wakes silent for power saving. Reset the board and type `?` during the cold-boot console window.

## Build and Upload

The project is configured in `platformio.ini`:

```text
Environment: esp32dev
Platform: espressif32
Framework: Arduino
Monitor speed: 115200
Library: 4-20ma/ModbusMaster
```

Build or upload from PlatformIO in VS Code. If automatic upload cannot enter the ESP32 bootloader, hold the board's BOOT-0 button while starting the upload and release it once flashing begins.

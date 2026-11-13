This was done in 2026 by Santiago Estrada (26Estrada)

<span style="color: green;">AI Assisted</span>

# Sensor – River Bed Monitoring System (MOLONARI 1D)

This README documents the sensor firmware in `main.cpp` and `main.hpp`. The
sensor runs on an Arduino MKR WAN 1310 and measures configured sensors, stores
the readings on an SD card, and sends the stored data to the relay over LoRa.

> **Do not use `sensor.ino`.** It is a legacy Arduino IDE entry point and is
> not part of the current firmware workflow. The project uses PlatformIO
> exclusively, with the `main_sensor` environment from `platformio.ini`.

## Summary

The sensor firmware:

- Loads its runtime configuration from `conf.csv` on the SD card.
- Measures each configured sensor at the configured interval.
- Stores measurements in `RECORDS.CSV`.
- Uses a LoRa handshake and sends unsent records to the relay.
- Retries later when records remain unsent.
- Accepts measurement and LoRa interval updates from the relay.
- Sleeps between measurement and communication events to reduce power use.

## Files and responsibilities

- `main.cpp` contains the firmware entry points:
  - `setup()` loads configuration, initializes sensors, the SD card, the RTC,
    the logger, and LoRa communication.
  - `loop()` measures sensors, logs data, communicates with the relay, checks
    for configuration updates, and sleeps until the next event.
- `main.hpp` contains firmware constants and defaults:
  - `Version::VERSION` is the sensor firmware version.
  - `Pins` defines the SD chip-select and sensor-enable pins.
  - `Storage` defines `conf.csv` and `RECORDS.CSV`.
  - `LoRaConfig`, `MeasurementConfig`, `RuntimeDefaults`, and `Timing` define
    communication, measurement, runtime, and timing defaults.
  - The `DEBUG_*` definitions enable the debug messages used by this firmware.

## Configuration file

Copy `conf.csv` to the root of the sensor's SD card. Empty lines and lines
starting with `#` are ignored.

### General settings

Use the following two-key format:

```csv
intervalle_de_mesure_secondes,80
lora_intervalle_secondes,240
```

- `intervalle_de_mesure_secondes` controls the measurement interval.
- `lora_intervalle_secondes` controls the LoRa communication interval.
- The current checked-in configuration measures every 80 seconds and
  communicates every 240 seconds. Change these values in the SD-card copy when
  a different schedule is required.

### Sensor settings

Each sensor uses three comma-separated fields:

```csv
id,type,pin
P1,pression,A0
T1,temperature,A1
```

The identifier is retained in the configuration, the type describes the
sensor, and the pin can be an Arduino analog name such as `A0` or a numeric
pin value. Add one line for every sensor to be initialized.

### Communication settings

The sensor configuration can also contain:

```csv
appEui,0862fe18fa55fb01
devEui,6ba823f81a8819e6
datalogger_slot,0
time_window,40
CSPin,5
lora_freq,868000000
```

- `devEui` identifies the sensor datalogger.
- `appEui` identifies the relay.
- `datalogger_slot` selects this datalogger's communication slot.
- `time_window` defines the slot duration in seconds.
- `lora_freq` overrides the default LoRa frequency when greater than zero.
- `CSPin` is accepted by the configuration parser, but the sensor's current
  startup code uses `Pins::SD_CHIP_SELECT_PIN` from `main.hpp` (currently pin
  5) when mounting the SD card.

Unknown keys are reported and ignored by `Reader`. The current parser also
accepts `appKey`; it is not used by the sensor runtime.

## Runtime flow

### 1. Startup

`setup()`:

1. Starts the serial monitor at 115200 baud.
2. Reads `conf.csv` from the SD card.
3. Applies valid intervals, EUIs, LoRa frequency, slot, and window values.
4. Creates one `Sensor` object for every configured sensor.
5. Initializes the SD card, RTC, data logger, and LoRa communication.

If the SD card cannot be initialized, startup stops.

### 2. Measurement and logging

When the measurement interval is reached, the loop reads every configured
sensor through `Sensor::get_voltage()`. The readings are stored in memory and
then appended to `RECORDS.CSV` by the `Writer` logger.

### 3. LoRa transmission

When this sensor's aligned communication slot is reached, the firmware:

1. Opens `RECORDS.CSV`.
2. Starts LoRa and performs a handshake with the relay.
3. Reads records from the last saved SD-card offset.
4. Sends the records and updates the offset after successful transmission.
5. Leaves `rattrapage` enabled when records remain, so they can be retried.
6. Checks for measurement and LoRa interval updates from the relay.

If the handshake fails, the file and LoRa connection are closed and the
records remain available for a later attempt.

### 4. Low-power sleep

After processing, the firmware calculates the time until the next measurement
and the next aligned communication slot. It sleeps until whichever event comes
first by calling `Waiter::sleepFor()`.

## Build, upload, and use

Run these commands from the firmware directory:

```bash
pio run -e main_sensor
pio run -e main_sensor -t upload
```

Alternatively, open the firmware directory in VS Code, select the PlatformIO
`main_sensor` environment, and use the Build and Upload actions.

Before booting the board:

1. Build or upload with PlatformIO; do not open or upload `sensor.ino`.
2. Copy the desired `conf.csv` to the root of the SD card.
3. Insert the SD card into the Arduino MKR WAN 1310.
4. Open the PlatformIO serial monitor at 115200 baud to inspect debug output.

<span style="color: green;">End of AI</span>


If you got until here, I formally apologise for the use of AI ;(
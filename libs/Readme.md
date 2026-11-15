Updated in 2026 by Nabil El Houssein (26elhousse)

# **Presentation of libraries**

This document presents the various libraries used in the Molonari project for In the Air applications, as well as their basic operation with diagrams. For a detailed explanation of a specific library, check the `README.md` inside that library's folder.

## *LoRa_Molonari*
LoRa_Molonari is a library enabling point-to-point wireless communication between dataloggers (`SLAVE`) and the relay (`MASTER`) via the LoRa protocol:

- Initialization and management of the LoRa connection: Starting and stopping the LoRa module, and switching the expected datalogger address (`setAddressWaited()`).
- Sending and receiving packets: With XOR checksum, acknowledgment management, and SD byte-offset tracking (`sendAllPacketsAndManageMemory()`).
- Timestamp injection: Injects the relay's RTC reception date and time into received CSV lines (`receiveAllPackets()`).
- Handshake: Establishing a reliable connection between a master and a slave.
- Session closure: Clean termination of communication (`closeSession()`).

           ┌────────────────────┐
           │ startLoRa()        │
           │ setAddressWaited() │
           └─────────┬──────────┘
                     │
                     ▼
           ┌────────────────────┐
           │ Handshake MASTER   │
           │ SYN → SYN-ACK → ACK│
           └─────────┬──────────┘
                     │
                     ▼
           ┌────────────────────────────────────┐
           │ sendAllPacketsAndManageMemory()    │
           │  - sendPacket(DATA)                │
           │  - wait for matching ACK           │
           │  - update SDOffset                 │
           └─────────┬──────────────────────────┘
                     │
                     ▼
           ┌────────────────────────────────────┐
           │ receiveAllPackets()                │
           │  - receivePacket()                 │
           │  - insert relay reception date/time│
           │  - send ACK (original payload)     │
           └─────────┬──────────────────────────┘
                     │
                     ▼
           ┌────────────────────┐
           │ closeSession()     │
           │  - send FIN        │
           │  - wait for FIN    │
           └────────────────────┘


## *LoRaWan_Molonari*
LoRaWan_Molonari is a library that enables data stored on the relay's SD card to be encoded into Protobuf and sent via the LoRaWAN network:

- OTAA connection: Activation of the LoRaWAN module (`EU868`) via `appEui` and `appKey`.
- Binary encoding: Parses CSV measurement lines and encodes them into compact Nanopb binary payloads (`encode()`).
- Data transmission & SD management: Sends queued `memory_line` packets with 5-second duty-cycle spacing and updates `SDOffset` as packets are acknowledged (`sendAllPacketsAndManageMemoryWAN()`).
- Error handling: Automatic retry (up to 3 times per wakeup window) and SD cursor rewind on failure.
- Network configuration: Adaptive data rate (ADR) and poll interval.

           ┌──────────────────────────┐
           │ Queue of memory_line     │
           │ (from RECORDS.CSV)       │
           └────────────┬─────────────┘
                        │
                        ▼
           ┌──────────────────────────────────┐
           │ LoraWANCommunication             │
           │ sendAllPacketsAndManageMemoryWAN │
           └────────────┬─────────────────────┘
                        │
                        ▼
           ┌──────────────────────────┐
           │ Loop on every msg        │
           │  - encode() to Protobuf  │
           │  - beginPacket()         │
           │  - write(buffer, length) │
           │  - endPacket(true)       │
           └────────────┬─────────────┘
                        │
                        ▼
           ┌──────────────────────────┐
           │ Check error              │
           │  - if ok : pop & advance │
           │    SDOffset (+ 5s delay) │
           │  - else : retry (max 3)  │
           └────────────┬─────────────┘
                        │
                        ▼
           ┌──────────────────────────┐
           │ Transmission success ?   │
           │  - Yes → next packet     │
           │  - No  → seek(SDOffset)  │
           │          & retry next    │
           │          cycle           │
           └──────────────────────────┘


## *encode*
encode is the Nanopb (Protocol Buffers) library used by `LoRaWan_Molonari` to serialize measurement lines into compact binary packets (`<= 51` bytes for `EU868` `DR0`):

- Defines the `SensorData` C struct and Nanopb field descriptors (`encode.h` / `encode.c`).
- Packs the 16-char datalogger `UI` (Tag `1`), sensor recording Unix `time` (Tag `2`), `relay_receive_time` (Tag `4`), and up to 10 raw 12-bit integer `measurements` (Tag `5`).

           ┌────────────────────────────────────────┐
           │ CSV Line from Relay SD Card            │
           │ UI ; id ; date ; time ; r_date ; ...   │
           └───────────────────┬────────────────────┘
                               │
                               ▼
           ┌────────────────────────────────────────┐
           │ SensorData Struct (encode.h)           │
           │  - Tag 1: UI (char[17])                │
           │  - Tag 2: time (int32_t Unix)          │
           │  - Tag 4: relay_receive_time (int32_t) │
           │  - Tag 5: measurements (uint32_t[10])  │
           └───────────────────┬────────────────────┘
                               │
                               ▼
           ┌────────────────────────────────────────┐
           │ pb_encode()                            │
           │  → Binary payload (~44 bytes <= 51B)   │
           └────────────────────────────────────────┘


## *Measure*
Measure is a library that allows you to retrieve data from sensors and format it:

- a `Sensor` class that retrieves data from physical analog sensors (12-bit ADC resolution, `0–4095`)
- a `Measure` class that formats the measurements sent to it

The data is returned in the form `"uidString ; id ; date ; time ; val1 ; val2 ; ... ;"` and is then sent to `Writer`.

                      ┌───────────────────────────┐
                      │        Sensor             │
                      └─────────────┬─────────────┘
                                    │
                 ┌──────────────────┴──────────────────┐
                 │                                     │
                 ▼                                     ▼
        ┌────────────────────┐                 ┌─────────────────────┐
        │ Default constructor│                 │ Full constructor    │
        │                    │                 │ with pins, 12-bit   │
        │ (dataPin=-1,...)   │                 │ ADC resolution      │
        └─────────┬──────────┘                 └─────────┬───────────┘
                  │                                      │
                  └──────────────┐───────────────────────┘
                                 ▼
                        ┌────────────────────────────────────┐
                        │ Sensor::get_voltage()              │
                        │ 1. Activate sensor (enablePin HIGH)│
                        │ 2. Wait 200 ms                     │
                        │ 3. Read analog on dataPin          │
                        │ 4. Deactivate sensor (LOW)         │
                        │ 5. Return voltage reading          │
                        └─────────┬──────────────────────────┘
                                  │
                                  ▼
                        ┌─────────────────┐
                        │   Measure       │
                        └─────────┬───────┘
                                  │
                    ┌─────────────┴─────────────┐
                    ▼                           ▼
        ┌───────────────────────┐     ┌───────────────────────┐
        │ Measure::oneLine()    │     │ Measure::ToString()   │
        │ - Gets RTC date/hour  │     │ - Calls oneLine()     │
        │ - Loops channel[]     │     │ - Returns CSV string  │
        │   for formatting      │     └───────────────────────┘
        │ - Returns CSV line    │
        └───────────────────────┘


## *Writer*
Writer is a library that allows you to write data to the SD card (`RECORDS.CSV`):

- The target CSV path is provided when constructing a `Writer`
- Connection management (automatic reconnection in case of loss)
- Optional debug logs
- Sequential IDs continued from the last stored measurement (`GetNextId()` seeks to the end of the file)
- Standardized CSV formatting (`LogData` on dataloggers, `LogString` on the relay) and flushing to the card

       ┌───────────────────────────────┐
       │          CAPTEURS             │
       │ (Measure via Sensor/Measure)  │
       └─────────────┬─────────────────┘
                     │
                     ▼
       ┌───────────────────────────────┐
       │          MEASURE              │
       │ - Contains values             │
       │ - Formats as CSV (ToString)   │
       └─────────────┬─────────────────┘
                     │
                     ▼
       ┌───────────────────────────────┐
       │           WRITER              │
       │                               │
       │ EstablishConnection()         │
       │ └─ GetNextId() (from EOF)     │
       │                               │
       │ LogData() / LogString()       │
       │ ├─ Check SD connection        │
       │ ├─ Reconnect() if lost        │
       │ ├─ Write CSV and flush()      │
       │ └─ Increments next_id         │
       │                               │
       │ Dispose() → Close file        │
       └─────────────┬─────────────────┘
                     │
                     ▼
       ┌───────────────────────────────┐
       │            SD                 │
       │ - Keeps measures in           │
       │   RECORDS.CSV                 │
       └───────────────────────────────┘


## *Reader*
Reader is a library that parses the `conf.csv` configuration file from the SD card at startup and provides the `memory_line` struct used to track SD byte offsets:

- Read a CSV configuration file (`conf.csv`) for both the relay and sensor nodes (LoRa settings, `dataloggers_EUIs`, `datalogger_slot`, `time_window`, sensors, etc.)
- Provide `memory_line(flush, memory_successor)` to track the exact SD card byte offset after each line in `RECORDS.CSV`
- *(Note: older methods like `UpdateCursor()` and `loadDataIntoQueue()` are kept in the class for legacy reasons but are replaced by `memory_line` and `lastSDOffset` in the active firmware.)*

                           ┌─────────────────────────────┐
                           │        Reader               │
                           └─────────────┬───────────────┘
                                         │
                ┌────────────────────────┴────────────────────────┐
                ▼                                                 ▼
       ┌────────────────────────────────────┐            ┌──────────────────────────────┐
       │ lireConfigCSV()                    │            │ memory_line struct           │
       │ - Opens conf.csv on SD             │            │ - flush (CSV line string)    │
       │ - Ignores '#' comments & empty     │            │ - memory_successor (SD byte  │
       │ - 2 tokens → Relay & Interval cfg  │            │   offset after the line)     │
       │ - 3 tokens → SensorConfig vector   │            └──────────────────────────────┘
       └────────────────────────────────────┘


## *Time & RtcBattery*
`Time` manages the external `RTC_PCF8523` clock and the TDMA slot alignment schedules for the MKR boards, while `RtcBattery` verifies the RTC hardware and backup battery at startup.

During `InitialiseRTC()`, `VerifyRtcBattery()` checks the RTC connection over I2C, verifies the Oscillator Stop (`OS`) flag, and checks the coin-cell battery voltage (`2.5V` threshold).

`GetCurrentDate()`, `GetCurrentHour()`, and `GetSecondsSinceMidnight()` read the current value from the external RTC. `CalculateElapsedSeconds()` compares timestamps and adds one day (`86400s`) when crossing midnight.

For communication scheduling, `IsInAlignedSlotWindow()` and `CalculateSleepTimeUntilNextAlignedSlot()` align the relay and dataloggers to exact multiples of `lora_intervalle_secondes` (with a `+1000 ms` wakeup margin) so multiple dataloggers can transmit in dedicated `time_window` slots (`i * time_window`) without colliding.

                     ┌───────────────────────────┐
                     │  Read conf.csv            │
                     │  (intervals & time_window)│
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │ InitialiseRTC()           │
                     │ └─ VerifyRtcBattery()     │
                     └─────────────┬─────────────┘
                                   │
             ┌─────────────────────┴─────────────────────┐
             ▼                                           ▼
    ┌──────────────────────┐                   ┌───────────────────────────┐
    │ GetCurrentDate/Hour()│                   │ GetSecondsSinceMidnight() │
    │  → stamps CSV lines  │                   └─────────┬─────────────────┘
    └──────────────────────┘                             │
                                                         ▼
                                   ┌─────────────────────┴─────────────────────┐
                                   ▼                                           ▼
                    ┌─────────────────────────────┐             ┌─────────────────────────────┐
                    │ CalculateSleepTimeUntil     │             │ IsInAlignedSlotWindow() &   │
                    │ NextMeasurement()           │             │ CalculateSleepTimeUntil     │
                    │  → sensor measurement loop  │             │ NextAlignedSlot() (+1000ms) │
                    └─────────────────────────────┘             │  → TDMA LoRa slot sync      │
                                                                └─────────────────────────────┘


## *Waiter*
This library manages timed waits for the Arduino.

    ┌─────────────────────────┐
    │       Waiter            │
    └────────────┬────────────┘
                 │
                 ▼
          ┌──────────────────────────────┐
          │ sleepFor(duration)           │
          │ - skips non-positive         │
          │   durations                  │
          │ - DEBUG on  → delay()        │
          │ - DEBUG off → LowPower       │
          │   deepSleep()                │
          └──────────────────────────────┘

`sleepFor()` accepts a duration in milliseconds. When `DEBUG` is enabled in `Waiter.cpp`, it uses `delay()` so the USB serial connection is not severed during debugging; when `DEBUG` is commented out for field deployment, it enters `LowPower.deepSleep()`.


## *Memory_monitor*
A lightweight debugging utility (`Memory_monitor.hpp` / `Memory_monitor.cpp`) that provides `freeMemory()` to inspect remaining RAM on the Arduino.

    ┌─────────────────────────┐
    │    Memory_monitor       │
    └────────────┬────────────┘
                 │
                 ▼
          ┌──────────────────────────────┐
          │ freeMemory()                 │
          │ - gets stack top (&top)      │
          │ - gets heap top (sbrk(0))    │
          │ - returns free bytes between │
          │   stack and heap             │
          └──────────────────────────────┘
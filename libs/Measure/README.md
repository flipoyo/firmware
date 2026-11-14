Written in 2026 by Nabil El Houssein (26elhousse)

**Measure & Sensor Library (`v1.0.3`)**

This library (`Measure.hpp` and `Measure.cpp`) is used by the datalogger to read the physical sensors plugged into the Arduino and format those readings into a single CSV line ready to be saved on the SD card. It splits that job into two classes: `Sensor` (which talks to the hardware pins) and `Measure` (which builds the text line).

**1. Reading a physical sensor (`Sensor`)**

Each configured sensor in `conf.csv` gets its own `Sensor` object initialized with `Sensor(dataPin, enablePin, type_capteur)`:
* In the constructor, it sets `enablePin` as an `OUTPUT`, `dataPin` as an `INPUT`, and configures the Arduino MKR WAN 1310's ADC to 12-bit resolution (`analogReadResolution(12)`), meaning readings range from `0` to `4095`.
* When `get_voltage()` is called, the Arduino doesn't keep the sensors powered all the time (that would drain the battery). Instead, it pulls `enablePin` `HIGH`, waits `200 ms` for the analog signal to stabilize, reads the raw 12-bit value with `analogRead(dataPin)`, and immediately turns `enablePin` back `LOW` before returning the value.

**2. Formatting a measurement line (`Measure`)**

Once the datalogger has read all its sensors into a `std::vector<double>`, it creates a `Measure(ncapt, toute_mesure, uidString)` object:
* The constructor automatically grabs the current date and time from the RTC (`GetCurrentDate()` and `GetCurrentHour()`), checks that `ncapt` isn't negative or larger than the vector size, and stores the readings in `channel`.
* Calling `oneLine()` (or `ToString()`, which just calls `oneLine()`) builds the semicolon-separated string that gets written to `RECORDS.CSV` on the datalogger's SD card:
  `uidString ; id ; date ; time ; val1 ; val2 ; ... ;`
  (Note that later on, when the relay receives this line over LoRa, `LoRa_Molonari` will insert the relay's reception date and time right after `time ;`).
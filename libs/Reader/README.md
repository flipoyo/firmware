Written in 2026 by Nabil El Houssein (26elhousse)

**Reader Library (`v1.2.0`)**

This library (`Reader.hpp` and `Reader.cpp`) is used at startup by both the relay and the datalogger to parse `conf.csv` from the SD card, and it also defines the `memory_line` struct used across the firmware to track which lines of `RECORDS.CSV` have already been sent.

**1. Reading `conf.csv` (`lireConfigCSV`)**

When `reader.lireConfigCSV(NomFichier, CSPin)` is called in `setup()`, it mounts the SD card, opens `conf.csv`, and reads it line by line (automatically skipping empty lines and comments starting with `#`). It splits each line by commas (`,`) and looks at how many tokens it found:

* **2 tokens (`key,value`):**
  * Stored inside `res.rel_config`: `appEui`, `appKey`, `CSPin`, `lora_freq`, `datalogger_slot`, and `time_window`.
  * **Special case for `devEui`:** Every time the parser sees a `devEui` line, it does two things at once: it overwrites `res.rel_config.devEui = val` (which the datalogger uses since it only has one `devEui`) AND it pushes `val` into the `res.rel_config.dataloggers_EUIs` vector (which the relay uses to loop through multiple dataloggers in order).
  * Stored inside `res.int_config`: `intervalle_de_mesure_secondes` and `lora_intervalle_secondes`.
* **3 tokens (`id,type,pin`) — Sensor Pins:**
  * Creates a `SensorConfig` entry and pushes it into `res.liste_capteurs`. If the pin starts with `"A"` (like `"A0"` or `"A6"`), it converts the string into the actual Arduino analog pin constant (`tokens[2].substring(1).toInt() + A0`).

If the SD card fails to mount or `conf.csv` is missing, `res.succes` is set to `false` so `main.cpp` knows to stick to the default constants in `main.hpp`.

**2. Tracking SD card progress (`memory_line`)**

At the bottom of `Reader.hpp`, you'll find a tiny struct that is super important for `LoRa_Molonari` and `LoRaWan_Molonari`:
* `flush`: The actual CSV text line read from `RECORDS.CSV`.
* `memory_successor`: The exact byte offset on the SD card (`dataFile.position()`) right after that line was read.

Whenever a packet is acknowledged over LoRa or LoRaWAN, the firmware sets `lastSDOffset = packet.memory_successor`. That way, if the next packet fails, the Arduino knows the exact byte position in `RECORDS.CSV` where it needs to resume on the next cycle.

**3. Legacy functions to ignore**

The remaining methods in `Reader` (`writetomyrecourdfile()`, `UpdateCursor()`, `loadDataIntoQueue()`, `ReadMeasure()`, `IsDataAvailable()`, and `Dispose()`) are leftover helpers from an older version of the code that tried to track line cursors in a separate `cursor_position.txt` file. They aren't used anymore by the relay or datalogger `main.cpp` (which now use `memory_line` and `lastSDOffset` directly).
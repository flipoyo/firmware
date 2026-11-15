Written in 2026 by Nabil El Houssein (26elhousse)

**Writer Library (`v1.2.0`)**

This library (`Writer.hpp` and `Writer.cpp`) handles writing measurement lines to `RECORDS.CSV` on the SD card. Both the dataloggers and the relay use it, though each calls a different logging method depending on where the data comes from.

**1. Setup & Auto-Resuming IDs (`EstablishConnection` & `GetNextId`)**

In `setup()`, the firmware creates a `Writer logger("RECORDS.CSV")` object and calls `logger.EstablishConnection(CSPin)`:
* It initializes the SD card and opens `RECORDS.CSV` in `FILE_WRITE` (append) mode.
* Before opening the file for writing, it calls `GetNextId()` so that if a datalogger reboots or loses power in the field, its measurement counter (`next_id`) doesn't reset back to `0`. Instead of reading the entire CSV from the top (which would get slow over months of logging), `GetNextId()` seeks straight to the very end of the file, walks backward past any trailing `\r` or `\n` characters to find the start of the last line, extracts the ID between the first and second semicolons, and returns `lastId + 1`.

**2. Writing Data (`LogData` vs. `LogString`)**

* **On the datalogger (`LogData(ncapteur, toute_mesure, uidString)`):** Takes the vector of sensor readings, wraps them into a `Measure` object stamped with `this->next_id`, appends the formatted line to `RECORDS.CSV` via `WriteInNewLine()`, calls `file.flush()` immediately so nothing gets lost in RAM if the battery dies, and increments `next_id`.
* **On the relay (`LogString(receiveQueue)`):** Takes the queue of already-formatted CSV strings received over LoRa from the dataloggers (which already have the relay's reception timestamp injected by `LoRa_Molonari`) and writes them directly to `RECORDS.CSV` using `file.print(payload)` followed by `file.flush()`. (Notice it uses `print` instead of `println` because the received lines already end with a newline character.)

**3. Automatic SD Reconnection (`Reconnect`)**

Before writing any line, both `LogData()` and `LogString()` check `SD.begin(CSPin) && this->file`. If the SD card connection dropped (for example, due to vibration or a brief power dip), it automatically calls `Reconnect()` to close the broken handle, re-mount the SD card, and reopen `RECORDS.CSV` before writing.
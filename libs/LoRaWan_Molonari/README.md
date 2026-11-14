Written in 2026 by Nabil El Houssein (26elhousse)

**LoRaWan_Molonari Library (`v3.0.1`)**

Once the relay has collected measurements from the dataloggers over LoRa and saved them into `RECORDS.CSV`, this library (`LoRaWan_Molonari.cpp` and `LoRaWan_Molonari.hpp`) takes over to forward those lines to the ChirpStack gateway over LoRaWAN using the Arduino MKR WAN 1310's built-in Murata modem (`MKRWAN.h`).

**1. Joining the network (`begin`)**

Calling `loraWAN.begin(appEui, appKey)` initializes the modem on the European `EU868` band and connects to the gateway using OTAA (`modem.joinOTAA(appEui, appKey)`). Once joined, it enables Adaptive Data Rate (`modem.setADR(true)`) and sets the minimum polling interval to 60 seconds.

**2. Parsing CSV lines into Protobuf (`encode`)**

Before sending a line from `RECORDS.CSV`, `encode()` splits the semicolon-separated string and packs it into the Nanopb `SensorData` struct (see the `encode` library README for the byte layout):
* **Field `0` (`UI`)**: Copied into `msg.UI` (the datalogger's 16-char `devEui`).
* **Field `1` (`id`)**: The measurement index which is skipped during encoding to save bytes.
* **Fields `2` & `3` (`sensor_date` & `sensor_time`)**: Converted from `"DD/MM/YYYY"` and `"HH:MM:SS"` into a single 32-bit Unix timestamp (`msg.time`) by `convertDateAndTimeToUnixTime()`.
* **Fields `4` & `5` (`relay_date` & `relay_time`)**: The reception date and time (which `LoRa_Molonari` injected after the 4th semicolon) are also converted into a Unix timestamp (`msg.relay_receive_time`).
* **Fields `6+` (`measurements`)**: Converted to integers (`toInt()`) and stored in `msg.measurements` (up to 9–10 sensors).

Finally, `pb_encode()` writes the compressed binary packet into `outBuffer` and logs `"Binary payload (length N bytes) ready to send!"`.

**3. Sending packets and tracking SD progress (`sendAllPacketsAndManageMemoryWAN`)**

This is the main function called by the relay loop. For each line in `sendQueue`:
1. It encodes the CSV line into binary using `encode()`.
2. It sends the binary buffer as a confirmed uplink (`modem.endPacket(true)`), trying up to `max_retries = 3` times per wakeup window.
3. **Why the 5-second delays?** If an uplink fails, it waits `5000 ms` before retrying, and after a successful uplink, it also waits `5000 ms` before sending the next packet in the queue. This backoff is critical because the `EU868` band has strict duty-cycle (airtime) limits which means that if you spam the Murata modem without waiting, it silently rejects packets.
4. If the gateway acknowledges the packet (`err > 0`), the line is popped from the queue and `SDOffset` is updated to `packet.memory_successor`. If it still fails after 3 retries, it rewinds the SD file cursor (`dataFile.seek(SDOffset)`) and returns `false` so the relay breaks out of the loop and retries from that exact line on the next cycle.

*Note: `sendQueue()` is commented out in the `.cpp` file and is just a legacy plain-text sender that is no longer used.*
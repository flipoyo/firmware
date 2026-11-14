Written in 2026 by Nabil El Houssein (26elhousse)

**Encode Library (`v2.0.0`)**

When the relay sends data over LoRaWAN, it cannot send plain text CSV lines because LoRaWAN packets are tiny. Instead, `LoRaWan_Molonari` uses this library (`encode.h` and `encode.c`) to pack each measurement line into a compact binary format using **Nanopb (Protocol Buffers)**.

These files were originally generated from a `.proto` file and then edited directly in `encode.h` to add the relay reception timestamp and switch the sensor readings from floats to integers.

**What gets packed into a message (`SensorData`)?**

Even though the C struct is called `SensorData` (we kept the old name to avoid breaking imports across the project), it actually packs four things:
1. `UI` (Tag `1`): The 16-character `devEui` of the datalogger (+ 1 additional char to fit the `\0` at the end).
2. `time` (Tag `2`): When the datalogger took the measurement (Unix timestamp).
3. `relay_receive_time` (Tag `4`): When the relay received that packet over LoRa.
4. `measurements` (Tag `5`): An array of up to 10 raw ADC integer values (`0` to `4095` from the Arduino's 12-bit `analogRead`).

Why do the tag numbers jump from `2` to `4` and `5`? Because in Protobuf, the server's decoder identifies fields by their tag number, not by their variable name. Tag `3` used to be `measurements` back when they were floats. When we switched to integers and added `relay_receive_time`, we moved `measurements` to Tag `5` on purpose so an outdated server decoder would immediately throw an error instead of misreading integers as floats.

**Watch out for the 51-byte LoRaWAN limit**

In Europe (EU868), when the relay transmits at long range (`DR0` to `DR2`), **a LoRaWAN packet cannot exceed 51 bytes**. 

Right now, a full message with 6 sensors (1 pressure + 5 temperatures) takes **44 bytes** (`18` for `UI` + `6` for `time` + `6` for `relay_receive_time` + `14` for the 6 measurements), and each extra sensor adds about 2 bytes. Note that `SensorData_size = 100` at the bottom of `encode.h` is just the RAM buffer size on the Arduino, not the actual packet size.

If you ever add new fields to `SensorData`, check the `Binary payload (length N bytes)` message on the serial monitor to make sure it stays `<= 51` (and don't forget to update the decoder on receiver/server to match your new tags).

For the attention of 2A mineurs in 2026/2027

1. This year’s final version

Some things work:
We use Arduino MKR WAN 1310 boards.
PlatformIO works well and allows handling of both external and internal project libraries. The management of the platformio.ini file depends on how the project evolves, but it can probably be kept largely as is.
Everything is neatly organized; the structure is clear: libraries on one side and the main files for each board on the other.
Everything compiles correctly on the computer, and uploading to the boards works.
We are able to receive information from the board: SD initialization, configuration reading, measurement launching, and local storage.
We are able to make the relay board and the sensor board communicate: BE CAREFUL to always update the config files stored on the SD cards with the correct values (for example, the devEui and appEui for inter-board communication).
One relay can now support several sensor boards. Each sensor board has its own identifier, and the relay can receive and handle data from the different sensors.
We are able to send data from the relay to the gateway, from the gateway to the server, and from the server to the website.

2. Next steps

```text
This block is for me to reveal my deepest secret. It is hard for me to admit it but we did now test the pressure sensor AT ALL during 2026. We tried and failed to connect it. Time was ticking and we ended up not solving the issue. We hope you manage to use it (we don't even know if it was actually used before). This means that the equation found for calibration was not tested. Good luck!
```

There are several things planned for implementation that were designed this year:
- Implement downlink communication. This is still missing.
  - The downlink must start from the VPS, where the configuration change is requested or generated.
  - Toulouse is only a tunnel/transport point in this architecture. It must not be treated as the origin of the downlink or as the place where downlink logic is implemented.
  - The VPS-side service must forward the downlink through the available network path to the gateway, relay, and sensor boards.
  - Add downlink handling to the relay and sensor so they can receive a new configuration, validate it, and safely replace the copy stored on the SD card.
  - Plan for corrupted files, partial updates, device desynchronization, and loss of subsequent measurements. A robust update protocol should preserve the previous configuration until the new one has been received and validated.
- Continue improving the libraries when new features require it. The libraries have already been optimized and organized more clearly, but new functionality may require additional interfaces, shared helpers, or refactoring. Keep small reusable functions separate from larger building-block libraries.
- Keep the current interval-based time management as the working baseline. It currently works well for communication and measurements.
  - As a future option, explore predefined communication times instead of intervals, for example a fixed schedule of communication windows during the day.
  - Compare this approach with the current interval-based approach before changing the firmware, especially with respect to RTC drift and communication-slot alignment.

The measurement schedule does not need board synchronization; it already works well independently on each sensor. If fixed communication times are introduced, revisit RTC synchronization and drift handling as part of that work.

Know that we did not perform any field test... sorry. So if you do one (highly recommended), things that work in the lab/atelier/workshop may not work outside. That is part of the magic of electronics.


3. Anything else?
A few general notes:
It’s helpful to know how to code in C++; otherwise check basic tutorials online.
Understanding the project structure and the code takes time—good luck.
We hope everything will work!
Uploading to the boards is a real hassle on a Mac—do it on Windows.
When uploading the program to the boards, you must press the reset button twice when the message “Waiting for the new upload port...” appears; this reinitializes the board.
To check if the boards are functioning properly, they should blink regularly with an orange light.
About the clocks: the RTC clock is set to the time of the last COMPILATION, so once the code is uploaded, pressing reset sets it back to the compilation time → this can cause several hours of offset! At the beginning, do NOT press reset after uploading, and compile all codes at the same time. This remains relevant if fixed communication times are explored.

## Authors
I sadly don't know the previous authors that left us a bunch of work, so I will just start adding from this year's (2026) edition. Feel free to add your name in the list:

- Nabil El Houssein (26elhousse)
- Santiago Estrada (26estrada)

### Note: 
The list is unumbered so no one feels left out. 
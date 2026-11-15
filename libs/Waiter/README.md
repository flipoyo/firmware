Written in 2026 by Nabil El Houssein (26elhousse)

**Waiter Library (`v2.0.1`)**

This tiny library (`Waiter.hpp` and `Waiter.cpp`) wraps the Arduino's sleep logic into a single helper method: `waiter.sleepFor(waiting_interval)` (where `waiting_interval` is in milliseconds). Both the relay and the dataloggers call it at the end of their `loop()` to save battery until the next measurement or RTC communication window.

**How `sleepFor()` works**

1. If `waiting_interval <= 0`, it skips sleeping and returns immediately (so the Arduino never tries to cast a negative number into an `unsigned long` and sleep for 49 days).
2. Depending on whether `DEBUG` / `DEBUG_WAITER` is defined at the top of `Waiter.cpp`, it picks between two sleep modes:
   * **Debug mode (`#define DEBUG` enabled):** Calls standard `delay(waiting_interval_unsigned)`. Why? Because putting an Arduino MKR WAN 1310 into real hardware deep sleep cuts off its native USB port, which disconnects your laptop's serial monitor and stops you from reading the logs!
   * **Field / Production mode (`#define DEBUG` commented out):** Calls `LowPower.deepSleep(waiting_interval_unsigned)` from `ArduinoLowPower.h` to put the SAMD21 microcontroller into low-power deep sleep.

**Important warning before deploying in the field**

Right now, `#define DEBUG` is **uncommented** at the top of `Waiter.cpp`. Before flashing the Arduinos for an actual field deployment on battery, make sure to comment out `#define DEBUG` otherwise the boards will just sit in `delay()` and drain their batteries much faster!
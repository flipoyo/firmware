Written in 2026 by Nabil El Houssein (26elhousse)

**Time Library (`v1.3.0`)**

This library (`Time.hpp` and `Time.cpp`) manages the external Real-Time Clock (`RTC_PCF8523` via `RTClib.h`) on both the relay and the dataloggers. Because all Arduinos in the field share the same synchronized RTC time, this library is what allows the relay and multiple dataloggers to wake up in exact TDMA time slots without ever colliding over LoRa.

**1. Initializing the RTC and checking its battery (`InitialiseRTC`)**

Calling `InitialiseRTC(forceOverride)` in `setup()` delegates the startup check to `VerifyRtcBattery(externalRtc, forceOverride)` (from `RtcBattery.hpp`), which verifies that the external PCF8523 chip is connected, checks whether its oscillator stopped (`OS` flag), and checks that the coin-cell backup battery voltage is above the `2.5V` threshold.

**2. Reading dates, hours, and elapsed seconds**

* **`GetCurrentDate()` & `GetCurrentHour()`**: Return the current RTC date as `"DD/MM/YYYY"` and time as `"HH:MM:SS"` (using `UIntTo2DigitString()` to zero-pad single digits like `"05"`). These strings are used to stamp measurement lines in `Measure` and relay reception times in `LoRa_Molonari`.
* **`GetSecondsSinceMidnight()`**: Converts the current `HH:MM:SS` into a single `long` from `0` to `86399`.
* **`CalculateElapsedSeconds(currentTime, previousTime)`**: Computes `currentTime - previousTime` and handles the midnight rollover automatically by adding `86400` (`TIME_SECONDS_PER_DAY`) if the difference is negative.

**3. TDMA Slot Alignment (`IsInAlignedSlotWindow` & `CalculateSleepTimeUntilNextAlignedSlot`)**

In the older version of the firmware, the Arduinos used relative sleep timers (`CalculateSleepTimeUntilNextCommunication`), which caused the relay and dataloggers to slowly drift out of sync whenever a LoRa handshake took a few extra seconds. 

To fix that, `v1.3.0` locks every communication window to absolute multiples of `lora_intervalle_secondes` since midnight (`t % intervalSeconds == offsetSeconds`):
* **`IsInAlignedSlotWindow(intervalSeconds, offsetSeconds, toleranceSeconds)`**: Checks whether the current RTC second falls inside `[offsetSeconds, offsetSeconds + toleranceSeconds]` of the current cycle. For example, the relay passes `offsetSeconds = 0` and `toleranceSeconds = 10` to wake up at the start of the cycle, while a datalogger passes `offsetSeconds = datalogger_slot * time_window` so it only transmits during its assigned slot.
* **`CalculateSleepTimeUntilNextAlignedSlot(intervalSeconds, offsetSeconds)`**: Calculates how many milliseconds remain until the next aligned slot and adds a **`+1000 ms` safety margin** (`(waitSeconds * 1000L) + 1000L`). That extra 1 second is super important: the Arduino's internal `LowPower.deepSleep()` clock is slightly imprecise and can wake the board up a fraction of a second *before* the RTC second actually ticks over, which would make `IsInAlignedSlotWindow()` return `false`!

*(Note: `CalculateSleepTimeUntilNextMeasurement()` is still used by the datalogger to schedule sensor readings between LoRa slots, whereas `CalculateSleepTimeUntilNextCommunication()` is kept for backwards compatibility.)*
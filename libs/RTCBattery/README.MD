
# RTCBattery

## Overview
This library monitors the PCF8523 RTC backup coin cell (CR1220) and blocks system startup if the clock lost power or if the battery voltage drops below 2.5V.

At the beginning of the MOLONARI project (2026-2027), we (Lea, Santiago & Nabil 😎) ran into confusing timestamp bugs on our packets without realizing the RTC coin cell was dead and even swapped it with another defective battery while debugging. This issue inspired this library. 

`RTCBattery` library provides diagnostics for the battery. On startup, it checks the I2C bus, ensures the PCF8523 power switchover (responsible for switching between Arduino and battery power) and 2.5V comparator (threshold when time drifts) are active, and checks the Oscillator Stop (`OS`) flag. If an issue is found, the board halts and blinks the built-in LED until the user either sets a new UTC timestamp (`SET YYYY-MM-DD HH:MM:SS` using 24-h format) or explicitly bypasses the check (`FORCE OVERRIDE`) via the Serial Monitor.

## How we tested it
We tested the library with two physical batteries: a healthy 3V cell and a dead 0.1V cell. The healthy cell passed with `OK` across reboots, while the 0.1V cell triggered `POWER_LOST` on every power-cycle (even after setting the time) confirming that dead batteries are caught immediately and no longer get masked. Furthermore, we confirmed that removing a healthy battery (while the Arduino is turned off) produces a `POWER_LOST` error (because RTC stops without a power supply => wrong time). This is already sufficient to prevent deploying sensors or relays with dead batteries or corrupted clocks. 

## Possible test for future developers
We could not test the `< 2.5V` (`BATTERY_LOW`) warning yet since we didn't have a partially discharged coin cell on hand; to test that specific threshold later, we can either partially discharge a CR1220 (through a resistor) or connect a bench power supply set to 2.3V to the `VBAT` contact.
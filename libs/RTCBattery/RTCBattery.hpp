#ifndef RTC_BATTERY_HPP
#define RTC_BATTERY_HPP

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#define nVersion "1.0.0"

enum class RtcBatteryStatus {
  OK,           // Battery >= 2.5V and clock is running normally
  BATTERY_LOW,  // Coin cell dropped below 2.5V (BLF bit = 1 in the register Control_3)
  POWER_LOST,   // Oscillator stopped or power was completely lost (OS bit = 1)
  I2C_ERROR     // PCF8523 chip not responding at address 0x68
};

// Reads the hardware flags and returns the exact status without halting
RtcBatteryStatus CheckRtcBatteryStatus(RTC_PCF8523& rtc);

// Runs the check and halts the board if the battery is low/dead, unless forceOverride is true or the user types "FORCE OVERRIDE" in the Serial Monitor.
bool VerifyRtcBattery(RTC_PCF8523& rtc, bool forceOverride = false);

#endif
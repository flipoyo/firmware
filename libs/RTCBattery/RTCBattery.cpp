#include "RTCBattery.hpp"

#define DEBUG_RTC_BATTERY

#ifdef DEBUG_RTC_BATTERY
    #define DEBUG_LOG(msg) Serial.println(msg)
#else
    #define DEBUG_LOG(msg)
#endif

namespace {

    // These constants should stay here, and not in the header file, because they shouldn't be accessible anywhere but in this file
    constexpr uint8_t PCF8523_ADDRESS = 0x68;
    constexpr uint8_t PCF8523_CONTROL_3 = 0x02;     // The address of the third control register from the PCF8523 documentation
    constexpr uint8_t PM_BITS_MASK = 0xE0;          // Bits 7..5: Power Management mode
    constexpr uint8_t BLF_BIT_MASK = 0x04;          // Bit 2: Battery Low Flag (< 2.5V)

    // Helper function: safely reads 1 byte from a PCF8523 register with full I2C error checking
    bool ReadRegister(uint8_t reg, uint8_t& valOut) {
        Wire.beginTransmission(PCF8523_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission() != 0) {
            DEBUG_LOG("ERROR: I2C failure when selecting the register.");
            return false;
        }

        if (Wire.requestFrom(PCF8523_ADDRESS, (uint8_t)1) != 1) {
            DEBUG_LOG("ERROR: I2C failure when reading the single byte.");
            return false;
        }

        valOut = Wire.read();
        return true;
    }


    // Reads Control_3 (0x02) and ensures battery switchover + low-battery detection are enabled
    bool ReadAndEnableControl3(uint8_t& ctrl3Out) {
        
        if (!ReadRegister(PCF8523_CONTROL_3, ctrl3Out)) {
            return false;
        }

        // If PM[2:0] (bits 7..5) are not 000, enable standard switchover and low-battery detection
        if ((ctrl3Out & PM_BITS_MASK) != 0x00) {
            DEBUG_LOG("Standard power switchover and low-battery detection (< 2.5 V) are not activated. Trying to activate...");
            Wire.beginTransmission(PCF8523_ADDRESS);
            Wire.write(PCF8523_CONTROL_3);
            Wire.write(ctrl3Out & ~PM_BITS_MASK);           // Set bits 7..5 to 000
            if (Wire.endTransmission() != 0) {
                DEBUG_LOG("ERROR: I2C failure when enabling the standard power switchover and low-battery detection.");
                return false;
            }
            delay(5);               // Let hardware voltage comparator settle

            if (!ReadRegister(PCF8523_CONTROL_3, ctrl3Out)) {
                return false;
            }
        }
        DEBUG_LOG("Standard power switchover and low-battery detection (< 2.5 V) are activated.");
        return true;
    }
}

RtcBatteryStatus CheckRtcBatteryStatus(RTC_PCF8523& rtc) {
    if (!rtc.begin()) {
        return RtcBatteryStatus::I2C_ERROR;
    }

    uint8_t ctrl3 = 0;
    if (!ReadAndEnableControl3(ctrl3)) {
        return RtcBatteryStatus::I2C_ERROR;
    }

    // Check if the RTC oscillator stopped or lost power completely
    if (!rtc.initialized() || rtc.lostPower()) {
        return RtcBatteryStatus::POWER_LOST;
    }

    // Check Bit 2 (BLF): 1 means CR1220 voltage is below 2.5V
    if ((ctrl3 & BLF_BIT_MASK) != 0) {
        return RtcBatteryStatus::BATTERY_LOW;
    }

    return RtcBatteryStatus::OK;
}

bool VerifyRtcBattery(RTC_PCF8523& rtc, bool forceOverride) {
    RtcBatteryStatus status = CheckRtcBatteryStatus(rtc);

    if (status == RtcBatteryStatus::OK) {
        Serial.println("OK: RTC Battery >= 2.5V and the time is valid.");
        return true;
    }

    // Print specific failure reason

    // I2C error: the RTC does not answer, so the time is unusable. No override is possible. Retry every 3 seconds and continue only if the chip answers again.
    if (status == RtcBatteryStatus::I2C_ERROR) {
        Serial.println("ERROR: RTC PCF8523 cannot be found on the I2C bus. Check wiring. Override is not possible, retrying every 3 s.");
        pinMode(LED_BUILTIN, OUTPUT);
        unsigned long lastRetryMs = millis();
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            if (millis() - lastRetryMs >= 3000) {
                lastRetryMs = millis();
                if (CheckRtcBatteryStatus(rtc) != RtcBatteryStatus::I2C_ERROR) {
                    digitalWrite(LED_BUILTIN, LOW);
                    return VerifyRtcBattery(rtc, forceOverride);
                }
            }
        }
    }
    
    if (status == RtcBatteryStatus::POWER_LOST) {
        Serial.println("ERROR: RTC power was lost (or its oscillator stopped) while the Arduino was unplugged (battery dead, missing, or popped loose). The RTC time is now WRONG.");
        Serial.println("ACTION: Check/replace the CR1220 battery, then set the correct UTC time by typing `SET YYYY-MM-DD HH:MM:SS` using 24h format");
    }
    else if (status == RtcBatteryStatus::BATTERY_LOW) {
        Serial.println("WARNING: RTC battery is dying (< 2.5V). The time might still be valid, but this is not guaranteed: the clock may stop or drift.");
        Serial.println("ACTION: Replace the CR1220 battery now, then set the correct UTC time by typing `SET YYYY-MM-DD HH:MM:SS` using 24h format");
    }

    Serial.println("Documentation: https://learn.adafruit.com/adafruit-adalogger-featherwing/adafruit2-rtc-with-arduino");

    if (forceOverride) {
        Serial.println("Force override is active in code. Starting with current RTC time under YOUR responsibility.");
        return true;
    }

    if (status == RtcBatteryStatus::POWER_LOST) {
        Serial.println("Type 'SET YYYY-MM-DD HH:MM:SS' using 24h format to set the clock (error dismissed on next reboot), OR 'FORCE OVERRIDE' to start anyway (error remains on next reboot):");
    } 
    else {
        Serial.println("Type 'SET YYYY-MM-DD HH:MM:SS' using 24h format to set the clock (the warning remains until the battery is replaced), OR 'FORCE OVERRIDE' to start anyway:");
    }
    pinMode(LED_BUILTIN, OUTPUT);

    // Halt loop: blink built-in LED and wait for "FORCE OVERRIDE" on Serial
    String userResponse;
    while (true) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(300);

        if (Serial.available() > 0) {
            userResponse = Serial.readStringUntil('\n');
            userResponse.trim();

            if (userResponse == "FORCE OVERRIDE") {
                digitalWrite(LED_BUILTIN, LOW);
                if (status == RtcBatteryStatus::POWER_LOST) {
                    Serial.println("FORCE OVERRIDE: Starting with a WRONG RTC time. Timestamps will be wrong. The error will remain on next reboot.");
                }
                else { // Battery low case
                    Serial.println("FORCE OVERRIDE: Starting with a low RTC battery. The error will remain on next reboot.");
                }
                return true;
            }
            // Check if user typed "SET YYYY-MM-DD HH:MM:SS" using 24h format (23 characters total)
            else if (userResponse.startsWith("SET ") && userResponse.length() >= 23) {
                
                // Validate format separators
                if (userResponse[8] != '-' || userResponse[11] != '-' ||
                    userResponse[14] != ' ' || userResponse[17] != ':' ||
                    userResponse[20] != ':') {
                    Serial.println("Invalid format. Use 'SET YYYY-MM-DD HH:MM:SS' using 24h format");
                    continue;
                }

                int year = userResponse.substring(4, 8).toInt();
                int month = userResponse.substring(9, 11).toInt();
                int day = userResponse.substring(12, 14).toInt();
                int hour = userResponse.substring(15, 17).toInt();
                int min = userResponse.substring(18, 20).toInt();
                int sec = userResponse.substring(21, 23).toInt();


                // Validate date and time
                DateTime dt(year, month, day, hour, min, sec);

                // Just a simple check to make sure that the datetime is somewhat logical
                if (year < 2026 || !dt.isValid() ||
                    hour > 23 || min > 59 || sec > 59) {
                    Serial.println("Invalid date or time, try again.");
                    continue;
                }

                rtc.adjust(DateTime(year, month, day, hour, min, sec));
                digitalWrite(LED_BUILTIN, LOW);

                RtcBatteryStatus statusAfterSet = CheckRtcBatteryStatus(rtc);
                if (statusAfterSet == RtcBatteryStatus::OK) {
                    Serial.println("RTC time updated. RTC OK.");
                    return true;
                }
                if (statusAfterSet == RtcBatteryStatus::BATTERY_LOW) {
                    Serial.println("RTC time updated. WARNING: the battery is still below 2.5V, replace it soon. Starting.");
                    return true;
                }
                Serial.println("RTC still reports a problem after SET. Re-verifying...");
                return VerifyRtcBattery(rtc, false);
            }
        }
    }
}
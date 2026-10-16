#pragma once
#include <stdint.h>

// This file contains version information for the sensor firmware. The VERSION variable is just the concatenation of the three previous values for better performance on low-performace devices (like the Arduino). I.e., MAJOR.MINOR.PATCH

namespace SensorVersion {
    constexpr uint8_t MAJOR = 0;                    // to be incremented after a major change that requires an update on the gateway or server (not backward compatible)
    constexpr uint8_t MINOR = 1;                    // to be incremented after a change that is backward compatible
    constexpr uint8_t PATCH = 2;                    // to be incremented after each bug fix or logic adjustment

    constexpr const char* VERSION = "0.1.2";        // Hardcoding it even after defining the three previous values for better performance on low-performace devices (like the Arduino)
}
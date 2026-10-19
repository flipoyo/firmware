#pragma once

#include <stdint.h>

// Enable debug logs
#define DEBUG_MAIN
#define DEBUG_MEASURE
#define DEBUG_WRITER
#define DEBUG_READER

#ifdef DEBUG_MAIN
    #define DEBUG_LOG(msg) Serial.println(msg)
    #define DEBUG_LOG_NO_LN(msg) Serial.print(msg)
#else
    #define DEBUG_LOG(msg)
    #define DEBUG_LOG_NO_LN(msg)
#endif

namespace Version {
    constexpr const char* VERSION = "1.2.8";        // Hardcoding it even after defining the three previous values for better performance on low-performace devices (like the Arduino)
}

// Serial Configuration
namespace SerialConfig {
    constexpr uint32_t BAUD_RATE = 9600;
    constexpr uint32_t TIMEOUT_MS = 5000;       // Maximum wait time for serial at startup
}

// Default hardware pins
namespace Pins {
    constexpr uint8_t SD_CS = 5;                   // Default SD card Chip Select pin
}

// LoRa settings
namespace LoRaConfig {
    constexpr long FREQUENCY_Hz = 868e6;        // Main frequency band used by LoRa and LoRaWAN in Europe (868 MHz)
    constexpr int INITIAL_INTERVAL_SECONDS = 3600;
    constexpr uint32_t MIN_SLEEP_MS = 10000;    // Minimum time left before next window to send LoRaWAN packets
}

// SD card file paths
namespace Storage {
    constexpr const char* CONFIG_FILE = "conf.csv";             // File used to load the configuration into the relay
    constexpr const char RECORDS_FILE[] = "RECORDS.CSV";        // File used to save the received data from the sensor (or sensors after upscaling to include more than one sensor)
}

// Initial runtime state
namespace RuntimeDefaults {
    constexpr long INITIAL_LAST_LORA_SEND = 0;
    constexpr long INITIAL_LAST_ATTEMPT = 0;
    constexpr unsigned long INITIAL_SD_OFFSET_CONFIG = 0;
    constexpr unsigned long INITIAL_SD_OFFSET = 0;
    constexpr bool INITIAL_MODIFICATION_STATE = false;
    constexpr uint8_t INITIAL_DEVICE_ID = 0;
}
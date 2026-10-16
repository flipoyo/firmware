#pragma once
#include <stdint.h>

namespace Version {
    constexpr const char* VERSION = "1.1.3";        // Hardcoding it even after defining the three previous values for better performance on low-performace devices (like the Arduino)

}

namespace SerialConfig {
    constexpr uint32_t BAUD_RATE = 115200;          // Default serial communication speed.
}

namespace Pins {
    constexpr uint8_t LORA_CS = 18;                 // LoRa chip-select pin.
    constexpr uint8_t LORA_RST = 14;                // LoRa reset pin.
    constexpr int SD_CHIP_SELECT_PIN = 5;           // SD card chip-select pin.
    constexpr int SENSOR_ENABLE_PIN = 4;            // Pin used to power or enable sensors.
}

namespace Storage {
    #define SENSOR_DATA_FILE_NAME "RECORDS.CSV"
    constexpr char DATA_FILE_NAME[] = SENSOR_DATA_FILE_NAME; // File containing recorded measurements.
    constexpr const char* CONFIG_FILE_PATH = "conf.csv"; // Configuration file path.
}

namespace LoRaConfig {
    constexpr long FREQUENCY_HZ = 868000000;        // LoRa radio frequency.
    constexpr long INITIAL_LAST_SEND = 0;           // Initial timestamp of the last LoRa transmission.
    constexpr uint16_t INITIAL_INTERVAL = 0;        // Initial LoRa transmission interval.
    constexpr uint16_t INITIAL_RECEIVED_INTERVAL = 0; // Default interval received from the relay.
    constexpr long INTERVAL_TOLERANCE_SECONDS = 60; // Early-send margin for LoRa scheduling.
    constexpr uint8_t INITIAL_HANDSHAKE_ID = 0;    // Initial identifier used during the LoRa handshake.
    constexpr uint8_t INITIAL_SESSION_PACKET = 0;  // Default packet value when closing a LoRa session.
}

namespace MeasurementConfig {
    constexpr long SECONDS_IN_DAY = 86400;         // Number of seconds in one day.
    constexpr long INITIAL_LAST_MEASURE = 0;       // Initial timestamp of the last measurement.
    constexpr uint16_t INITIAL_INTERVAL = 0;       // Initial measurement interval.
    constexpr long INTERVAL_TOLERANCE_SECONDS = 1; // Measurement scheduling tolerance.
    constexpr unsigned long SOON_THRESHOLD_MS = 1000; // Threshold for deciding a measurement is imminent.
    constexpr double INITIAL_VALUE = 0;             // Default value stored for a new measurement.
    constexpr uint16_t INITIAL_RECEIVED_INTERVAL = 0; // Default measurement interval received from the relay.
}

namespace RuntimeDefaults {
    constexpr int INITIAL_SENSOR_COUNT = 0;        // Number of sensors before configuration loading.
    constexpr unsigned long INITIAL_SD_OFFSET = 0; // Initial read offset in the SD data file.
    constexpr bool INITIAL_RATTRAPAGE = false;     // Whether unsent data recovery is active initially.
    constexpr bool INITIAL_LINE_REMAINS_TO_LOG = false; // Whether a measurement is waiting to be logged.
    constexpr long INITIAL_CURRENT_TIME = 0;       // Initial current-time value.
}

namespace Timing {
    constexpr unsigned long SERIAL_STARTUP_TIMEOUT_MS = 5000; // Maximum wait for the serial port.
    constexpr unsigned long SETUP_DELAY_MS = 1000;  // Delay after setup completes.
    constexpr unsigned long CONFIG_UPDATE_TIMEOUT_MS = 15000; // Wait time for a configuration update.
    constexpr unsigned long SOON_THRESHOLD_MS = 1000; // Threshold for determining if a measurement is imminent.
}
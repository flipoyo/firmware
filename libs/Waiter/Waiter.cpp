#include <Arduino.h>
#include <ArduinoLowPower.h>

#include "Waiter.hpp"

#define DEBUG
#ifdef DEBUG
#define DEBUG_WAITER
#endif
#ifdef DEBUG_WAITER
#define DEBUG_LOG(msg) Serial.println(msg)
#else
#define DEBUG_LOG(msg) 
#endif

// Default constructor
Waiter::Waiter() {}

// Sleep the Arduino until the desired waiting time passes
void Waiter::sleepUntil(long waiting_interval) {

    if (waiting_interval <= 0) {
        DEBUG_LOG("Sleep skipped because the requested interval is not positive");
        return;
    }

    DEBUG_LOG("Sleeping for " + String(waiting_interval) + " ms");
    // Sleep for the calculated time
    unsigned long waiting_interval_unsigned = (unsigned long) waiting_interval;
    LowPower.deepSleep(waiting_interval_unsigned); //Conversion rendue nécessaire par l'implémentation de deepSleep dans la librairie arduino
}

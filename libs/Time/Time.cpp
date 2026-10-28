// Time.cpp
// This file defines functions for handling time and date using an external RTC module.


#include <RTClib.h>
#include <vector>

#include "Time.hpp"
#include "Reader.hpp"
#include "RtcBattery.hpp"

#define DEBUG_TIME
#ifdef DEBUG_TIME
  #define DEBUG_LOG(msg) Serial.println(msg)
#else
  #define DEBUG_LOG(msg)
#endif

namespace {
  const long TIME_SECONDS_PER_DAY = 24L * 60L * 60L;
}

// ----- RTC -----
RTC_PCF8523 externalRtc;
int measurementCount = 0;

// ----- Configuration -----
Reader reader;
int freq_envoi_lora_seconds = 0;
int freq_mesure_seconds = 0;

std::vector<unsigned long> measurementTimesVec;
std::vector<unsigned long> communicationTimesVec;


void InitialiseRTC(bool forceOverride) {
  // Check RTC connection, OS (Oscillator stopped) flag, and 2.5V battery threshold
  VerifyRtcBattery(externalRtc, forceOverride);
  DEBUG_LOG("RTC externe initialisée.");
}


String UIntTo2DigitString(uint8_t x) {
  String str = String(x);
  if (x < 10) str = "0" + str;
  if (x >= 100) str.remove(2);
  return str;
}

String GetCurrentDate() {
  DateTime now = externalRtc.now();
  return UIntTo2DigitString(now.day()) + "/" +
        UIntTo2DigitString(now.month()) + "/" +
        String(now.year());
}

String GetCurrentHour() {
  DateTime now = externalRtc.now();
  return UIntTo2DigitString(now.hour()) + ":" +
        UIntTo2DigitString(now.minute()) + ":" +
        UIntTo2DigitString(now.second());
}

long GetSecondsSinceMidnight() {
  DateTime now = externalRtc.now();
  return now.hour() * 3600 + now.minute() * 60 + now.second();
}

long CalculateElapsedSeconds(long currentTime, long previousTime) {
  long elapsedTime = currentTime - previousTime;
  if (elapsedTime < 0) {
    elapsedTime += TIME_SECONDS_PER_DAY;
  }
  return elapsedTime;
}

long CalculateSleepTimeUntilNextMeasurement(long previousMeasurementTime, int measurementInterval) {
  //retourne en ms
  long time_to_sleep = (measurementInterval - CalculateElapsedSeconds(GetSecondsSinceMidnight(), previousMeasurementTime))*1000;
  if(time_to_sleep <= 0){
    return time_to_sleep = 0;
  }
  return (time_to_sleep);
}


long CalculateSleepTimeUntilNextCommunication(long previousCommunicationTime, int communicationInterval) {
  //retourne en ms
  long time_to_sleep = (communicationInterval - CalculateElapsedSeconds(GetSecondsSinceMidnight(), previousCommunicationTime)) * 1000;
    if(time_to_sleep <= 0){
    return time_to_sleep = 0;
  }
  return (time_to_sleep);
}
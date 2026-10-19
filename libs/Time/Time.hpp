#ifndef MY_TIME
#define MY_TIME

#include <RTClib.h>
#include <vector>

#define nVersion 1.1.1

// ----- RTC -----
extern RTC_PCF8523 externalRtc;
extern std::vector<unsigned long> measurementTimesVec;
extern int measurementCount;

// ----- Fonctions -----
String UIntTo2DigitString(uint8_t x);

void InitialiseRTC();

String GetCurrentDate();
String GetCurrentHour();
long GetSecondsSinceMidnight();
long CalculateElapsedSeconds(long currentTime, long previousTime);

long CalculateSleepTimeUntilNextMeasurement(long previousMeasurementTime, int measurementInterval);
long CalculateSleepTimeUntilNextCommunication(long previousCommunicationTime, int communicationInterval);

#endif // MY_TIME

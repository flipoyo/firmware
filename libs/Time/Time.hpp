#ifndef MY_TIME
#define MY_TIME

#include <RTClib.h>
#include <vector>

#define nVersion 1.3.0

// ----- RTC -----
extern RTC_PCF8523 externalRtc;
extern std::vector<unsigned long> measurementTimesVec;
extern int measurementCount;

// ----- Fonctions -----
String UIntTo2DigitString(uint8_t x);

void InitialiseRTC(bool forceOverride = false);

String GetCurrentDate();
String GetCurrentHour();
long GetSecondsSinceMidnight();
long CalculateElapsedSeconds(long currentTime, long previousTime);

long CalculateSleepTimeUntilNextMeasurement(long previousMeasurementTime, int measurementInterval);
long CalculateSleepTimeUntilNextCommunication(long previousCommunicationTime, int communicationInterval);

bool IsInAlignedSlotWindow(int intervalSeconds, int offsetSeconds, int toleranceSeconds);
long CalculateSleepTimeUntilNextAlignedSlot(int intervalSeconds, int offsetSeconds = 0);

#endif // MY_TIME

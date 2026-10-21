// Measure.cpp
// Implementation of the Measure and Sensor classes, in order to handle sensor measurements.

#include<vector>
#include "Measure.hpp"
#include "Time.hpp"

#define DEBUG_MEASURE
#ifdef DEBUG_MEASURE
#define DEBUG_LOG(msg) Serial.println(msg)
#else 
#define DEBUG_LOG(msg)
#endif

// Constructeur par défaut
Sensor::Sensor() 
  : dataPin(-1), enablePin(4), type_capteur("-1") {}
// Constructeur complet
Sensor::Sensor(int _dataPin, int _enablePin, String _type_capteur)
  : dataPin(_dataPin), enablePin(_enablePin), type_capteur(_type_capteur)
{
  pinMode(enablePin, OUTPUT);
  pinMode(dataPin, INPUT);
  analogReadResolution(12); // précision maximale sur MKR
}


double Sensor::get_voltage() { 
  digitalWrite(enablePin, HIGH);
  delay(200);
  double voltage = analogRead(dataPin);
  DEBUG_LOG("measured voltage of Pin " + String(dataPin) + " of " + String (voltage));
  digitalWrite(enablePin, LOW);
  return voltage;
}

Measure::Measure(int ncapt_arg, const std::vector<double>& toute_mesure, String uidString_arg):
  id(0),
  date(GetCurrentDate()),
  time(GetCurrentHour()),
  time_in_second(0),
  ncapt(ncapt_arg),
  channel(),
  uidString(uidString_arg)
  {
    DEBUG_LOG("Taking measure at time : " + String(time) + "; and date : " + String(date) + ";");

  if (ncapt < 0) {
    DEBUG_LOG("Invalid negative sensor count; recording no channels");
    ncapt = 0;
  } else if (static_cast<size_t>(ncapt) > toute_mesure.size()) {
    DEBUG_LOG("Sensor count exceeds available measurements; recording available channels only");
    ncapt = static_cast<int>(toute_mesure.size());
  }
  
  for (int iterator = 0; iterator < ncapt; iterator ++){
    channel.push_back(toute_mesure[iterator]);
  }
}

// Retourne une ligne formatée pour une mesure
String Measure::oneLine() {
  DEBUG_LOG("starting oneLine");
  DEBUG_LOG(String(ncapt) + " capteurs détectés");
  DEBUG_LOG("Heure actuelle : " + time);
  // Construction de la ligne
  String str = this->uidString;
  str += " ; " + String(id) + " ; " + this->date + " ; " + this->time + "  ; ";

  // Ajouter les valeurs des capteurs
  const size_t valueCount = channel.size() < static_cast<size_t>(ncapt)
    ? channel.size()
    : static_cast<size_t>(ncapt);
  for (size_t i = 0; i < valueCount; i++) {
    str += " " + String(channel[i]) + "; ";
  }
  DEBUG_LOG("end of initialization of one LineMeasure" + str);
  return str;
}
// Retourne une version complète et lisible de la mesure
String Measure::ToString() {
  DEBUG_LOG("going to oneLine");
  String str = oneLine();
  DEBUG_LOG("oneLine finished");
  return str;
}
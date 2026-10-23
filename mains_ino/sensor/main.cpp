#include <queue>
#include <vector>
#include <Arduino.h>
#include <SD.h>
#include <LoRa.h>
#include <string>
#include <ArduinoLowPower.h>

#include "Measure.hpp"
#include "Writer.hpp"
#include "LoRa_Molonari.hpp"
#include "Time.hpp"
#include "Waiter.hpp"
#include "Reader.hpp"

#include "main.hpp"


Sensor** sens;
std::vector<double> toute_mesure;

GeneralConfig config;
std::vector<SensorConfig> liste_capteurs;
int lora_intervalle_secondes = LoRaConfig::INITIAL_INTERVAL;
int intervalle_de_mesure_secondes = MeasurementConfig::INITIAL_INTERVAL;
String devEui;
String appEui;


Writer logger(Storage::DATA_FILE_NAME);
int ncapt = RuntimeDefaults::INITIAL_SENSOR_COUNT;
long timeUntilNextMeasure;
long timeUntilNextCommunication;

// LoRa
LoraCommunication lora(LoRaConfig::FREQUENCY_HZ, devEui, appEui, RoleType::SLAVE);
long lastLoRaSend = LoRaConfig::INITIAL_LAST_SEND;
long lastMeasure = MeasurementConfig::INITIAL_LAST_MEASURE;
unsigned long lastSDOffset = RuntimeDefaults::INITIAL_SD_OFFSET;

uint16_t newMeasureInterval = MeasurementConfig::INITIAL_INTERVAL;
uint16_t newLoraInterval = LoRaConfig::INITIAL_INTERVAL;

bool rattrapage = RuntimeDefaults::INITIAL_RATTRAPAGE;
bool a_line_remains_to_log = RuntimeDefaults::INITIAL_LINE_REMAINS_TO_LOG;
Waiter waiter;

// ----- Setup -----
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Serial.begin(SerialConfig::BAUD_RATE);
    unsigned long end_date = millis() + Timing::SERIAL_STARTUP_TIMEOUT_MS;
    while (!Serial && millis() < end_date) {}
    
    DEBUG_LOG_NO_LN("Sensor Firmware Version: ");
    DEBUG_LOG(Version::VERSION);

    DEBUG_LOG("\n\n\n\n");
    // Lecture de la configuration CSV
    Reader reader;

    GeneralConfig temp_config_container = reader.lireConfigCSV(Storage::CONFIG_FILE_PATH, Pins::SD_CHIP_SELECT_PIN);
    long lora_frequency = LoRaConfig::FREQUENCY_HZ;

    if (temp_config_container.succes) {
        if (!temp_config_container.liste_capteurs.empty()) {
            liste_capteurs = temp_config_container.liste_capteurs;
        }
        if (temp_config_container.int_config.lora_intervalle_secondes > 0) {
            lora_intervalle_secondes = temp_config_container.int_config.lora_intervalle_secondes;
        }
        if (temp_config_container.int_config.intervalle_de_mesure_secondes > 0) {
            intervalle_de_mesure_secondes = temp_config_container.int_config.intervalle_de_mesure_secondes;
        }
        if (temp_config_container.rel_config.devEui.length() > 0) {
            devEui = temp_config_container.rel_config.devEui;
        }
        if (temp_config_container.rel_config.appEui.length() > 0) {
            appEui = temp_config_container.rel_config.appEui;
        }
        if (temp_config_container.rel_config.lora_freq > 0) {
            lora_frequency = static_cast<long>(temp_config_container.rel_config.lora_freq);
        }
    }

    if (temp_config_container.succes){
        DEBUG_LOG("lecture config terminée avec succès");
    }
    else {
        DEBUG_LOG("échec de la lecture du fichier config");
    }

    lora.LoraUpdateAttributes(lora_frequency, appEui, devEui, RoleType::SLAVE);

    // Compter les capteurs
    for (auto& _c : liste_capteurs) {
        ncapt++;
    }
    // Allocation dynamique
    sens = new Sensor*[ncapt];
    
    // Initialisation des capteurs
    for (int it = 0; it<ncapt; it++) {
        SensorConfig _c = liste_capteurs[it];
        sens[it] = new Sensor(_c.pin, Pins::SENSOR_ENABLE_PIN, _c.type_capteur);
        toute_mesure.push_back(MeasurementConfig::INITIAL_VALUE);
        DEBUG_LOG("inserted new Sensor ptr at position " + String(it) + " of sens with attributes :");
        DEBUG_LOG("_c.pin : " + String(_c.pin) + "  _c.type_capteur : " + String(_c.type_capteur) + "\n\n");
    }

    // Initialisation SD et logger
    if (!SD.begin(Pins::SD_CHIP_SELECT_PIN)) { while(true) {} }
    logger.EstablishConnection(Pins::SD_CHIP_SELECT_PIN);
    InitialiseRTC();
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    DEBUG_LOG ("Setup finished");
    delay(Timing::SETUP_DELAY_MS);
}

// ----- Loop -----
void loop() {

    // String date = GetCurrentDate();
    // String hour = GetCurrentHour();
    DEBUG_LOG(String(ncapt) + " capteurs détectés.");

    
    
    // --- Prendre mesures ---
    long current_Time=GetSecondsSinceMidnight();
    DEBUG_LOG("Temps actuel : " + String(current_Time));
    bool IsTimeToMeasure = (CalculateElapsedSeconds(current_Time, lastMeasure) >= (intervalle_de_mesure_secondes - MeasurementConfig::INTERVAL_TOLERANCE_SECONDS));
    
    DEBUG_LOG("ncapt : " + String(ncapt)); //so far

    if (IsTimeToMeasure) {
        DEBUG_LOG("starting measurement process");
        digitalWrite(LED_BUILTIN, HIGH);
        for (int it = 0; it<ncapt; it++) {
            double v = sens[it]->get_voltage();
            DEBUG_LOG("voltage preleved");
            toute_mesure[it] = v;
            DEBUG_LOG("Measured " + String(v) + " at sensor " + String(it));
            //delay(500);//decreased for demo version
        }
        lastMeasure = current_Time;
        DEBUG_LOG("Voltage recording finished");
        a_line_remains_to_log = true;
        digitalWrite(LED_BUILTIN, LOW);
    }else{
        DEBUG_LOG("Not time to measure yet : elapsed time = " + String(CalculateElapsedSeconds(current_Time, lastMeasure)) + " ; intervalle_de_mesure_secondes - tolerance = " + String(intervalle_de_mesure_secondes - MeasurementConfig::INTERVAL_TOLERANCE_SECONDS));
    }

    
    // --- Stocker sur SD ---
    if(a_line_remains_to_log){
        DEBUG_LOG("launch LogData");
        logger.LogData(ncapt, toute_mesure, devEui);
        DEBUG_LOG("LogData instruction done");
        a_line_remains_to_log = false;
    }

    // --- Envoyer LoRa si intervalle atteint ---
    
    bool IsTimeToLoRa = (CalculateElapsedSeconds(current_Time, lastLoRaSend) >= (lora_intervalle_secondes - LoRaConfig::INTERVAL_TOLERANCE_SECONDS));

    if (IsTimeToLoRa || rattrapage) {
        digitalWrite(LED_BUILTIN, HIGH);
        File dataFile = SD.open(Storage::DATA_FILE_NAME, FILE_READ);
        DEBUG_LOG("File has been opened");

        if (!dataFile) {
            DEBUG_LOG("Impossible d'ouvrir le fichier de données pour LoRa");
            lora.stopLoRa();
            return;
        }

        lora.startLoRa();
        DEBUG_LOG("LoRa ok, about to try handshake");

        uint8_t id = LoRaConfig::INITIAL_HANDSHAKE_ID;
        if (lora.handshake(id)) {
            DEBUG_LOG("HANDSHAKE DONE\n");
            timeUntilNextMeasure = CalculateSleepTimeUntilNextMeasurement(lastMeasure, intervalle_de_mesure_secondes);
            DEBUG_LOG("CalculateSleepTimeUntilNextMeasurement : " + String(timeUntilNextMeasure) + "ms");

            DEBUG_LOG(String(dataFile.available()));
            bool time_to_mesure_soon = timeUntilNextMeasure < Timing::SOON_THRESHOLD_MS;
            if(time_to_mesure_soon){
                DEBUG_LOG("Aborting LoRa send, measurement scheduled soon. Switching to rattrapage mode");
                lora.closeSession(LoRaConfig::INITIAL_SESSION_PACKET);
                rattrapage = true;
            }
            while ( (!time_to_mesure_soon) && dataFile.available()) { //racourcir de 60000 à 10000 pour les besoins de la démo                
                //at this point, lastSDOffset must point to the first memory address of the first line to be sent
                std::queue<memory_line> linesToSend;
                dataFile.seek(lastSDOffset);
                while (dataFile.available()) {
                    memory_line new_line = memory_line(dataFile.readStringUntil('\n'), dataFile.position());
                    new_line.flush = new_line.flush + "\n";
                    linesToSend.push(new_line);

                // Si la ligne est vide aka plus rien à envoyer
                    if (linesToSend.front().flush.length() == 0) {
                        break;
                    }
                }
                uint32_t end_document_address = dataFile.position();
                DEBUG_LOG("end_document_address : " + String(end_document_address) + "; memory_successor of the last element of sendqueue : " + String(linesToSend.back().memory_successor));
                DEBUG_LOG("SDOffset before sending: " + String(lastSDOffset) + "\n                     SEND ALL PACKETS AND MANAGE MEMORY");
                uint8_t lastPacket = lora.sendAllPacketsAndManageMemory(linesToSend, lastSDOffset);
                DEBUG_LOG("SDOffset after sending: " + String(lastSDOffset) + "; end document address : " + String(end_document_address));
                rattrapage = !(lastSDOffset == end_document_address);
                DEBUG_LOG("rattrappage status : " + String(rattrapage) + "\n\n");
                lora.closeSession(lastPacket);
            }    // <-- fermeture du while : on a tout envoyé ou on va bientôt faire une mesure !

            dataFile.close();
            lastLoRaSend = current_Time;

            // --- Réception éventuelle de mise à jour config ---
            DEBUG_LOG("Vérification de mise à jour descendante...");


            uint16_t recvMeasure = MeasurementConfig::INITIAL_RECEIVED_INTERVAL;
            uint16_t recvLora = LoRaConfig::INITIAL_RECEIVED_INTERVAL;
            if (lora.receiveConfigUpdate(Storage::CONFIG_FILE_PATH, &recvMeasure, &recvLora, Timing::CONFIG_UPDATE_TIMEOUT_MS)) {
                DEBUG_LOG("Mise à jour config reçue du master.");
                intervalle_de_mesure_secondes = recvMeasure;
                lora_intervalle_secondes = recvLora;
            } else {
                DEBUG_LOG("Pas de mise à jour reçue.");
            }

            lora.stopLoRa();
            digitalWrite(LED_BUILTIN, LOW);
        } else {
            DEBUG_LOG("Handshake échoué");
            for (int blink = 0; blink < 5; blink++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN, LOW);
            delay(300);
            }

        }
        // --- Sommeil jusqu'à prochaine mesure ---
        DEBUG_LOG("waiter instancié");
        timeUntilNextMeasure = CalculateSleepTimeUntilNextMeasurement(lastMeasure, intervalle_de_mesure_secondes);
        timeUntilNextCommunication = CalculateSleepTimeUntilNextCommunication(lastLoRaSend, lora_intervalle_secondes);
        
        if (timeUntilNextMeasure <= timeUntilNextCommunication){
            long time_to_sleep = timeUntilNextMeasure;
            DEBUG_LOG("sleeping until next measure, sleeping for " + String (time_to_sleep)+ "ms");
            waiter.sleepFor(timeUntilNextMeasure);
        } else {
            DEBUG_LOG("sleeping until next communication " + String (timeUntilNextCommunication)+"ms");
            waiter.sleepFor(timeUntilNextCommunication);
        }

        #ifdef DEBUG_MAIN
        // Blink for when it wakes up from sleep, to indicate that it is awake and running the loop again
        for (int blink = 0; blink < 5; blink++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
        #endif

        digitalWrite(LED_BUILTIN, LOW);
    }
}